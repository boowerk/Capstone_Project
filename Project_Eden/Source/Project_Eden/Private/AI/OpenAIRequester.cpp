#include "AI/OpenAIRequester.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/PlatformMisc.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

static FString TrimAndStrip(const FString& Input)
{
	FString S = Input;
	S.TrimStartAndEndInline();

	if (S.StartsWith(TEXT("```")))
	{
		const int32 First = S.Find(TEXT("\n"));
		const int32 Last = S.Find(TEXT("```"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);

		if (First != INDEX_NONE && Last != INDEX_NONE && Last > First)
		{
			S = S.Mid(First + 1, Last - (First + 1));
			S.TrimStartAndEndInline();
		}
	}

	return S;
}

namespace
{
	FString SerializeJsonObject(const TSharedRef<FJsonObject>& JsonObject)
	{
		FString Payload;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
		FJsonSerializer::Serialize(JsonObject, Writer);
		return Payload;
	}

	FString BuildTextOnlyInputPayload(const FString& Model, const FString& Prompt, int32 MaxOutputTokens)
	{
		const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetStringField(TEXT("model"), Model);
		Root->SetNumberField(TEXT("max_output_tokens"), MaxOutputTokens);

		const TSharedRef<FJsonObject> InputMessage = MakeShared<FJsonObject>();
		InputMessage->SetStringField(TEXT("role"), TEXT("user"));

		const TSharedRef<FJsonObject> InputText = MakeShared<FJsonObject>();
		InputText->SetStringField(TEXT("type"), TEXT("input_text"));
		InputText->SetStringField(TEXT("text"), Prompt);

		TArray<TSharedPtr<FJsonValue>> ContentArray;
		ContentArray.Add(MakeShared<FJsonValueObject>(InputText));
		InputMessage->SetArrayField(TEXT("content"), ContentArray);

		TArray<TSharedPtr<FJsonValue>> InputArray;
		InputArray.Add(MakeShared<FJsonValueObject>(InputMessage));
		Root->SetArrayField(TEXT("input"), InputArray);

		return SerializeJsonObject(Root);
	}

	bool TryAppendTextField(const TSharedPtr<FJsonObject>& JsonObject, FString& InOutText)
	{
		if (!JsonObject.IsValid())
		{
			return false;
		}

		FString TextValue;
		if (JsonObject->TryGetStringField(TEXT("text"), TextValue))
		{
			InOutText += TextValue;
			return true;
		}

		const TSharedPtr<FJsonObject>* NestedTextObject = nullptr;
		if (JsonObject->TryGetObjectField(TEXT("text"), NestedTextObject) && NestedTextObject && NestedTextObject->IsValid())
		{
			if ((*NestedTextObject)->TryGetStringField(TEXT("value"), TextValue))
			{
				InOutText += TextValue;
				return true;
			}
		}

		return false;
	}

	bool ExtractResponseText(const FString& Body, FString& OutText)
	{
		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);

		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[OpenAI] Top JSON parsing failed"));
			return false;
		}

		FString TopLevelOutputText;
		if (Root->TryGetStringField(TEXT("output_text"), TopLevelOutputText))
		{
			OutText = TrimAndStrip(TopLevelOutputText);
			return !OutText.IsEmpty();
		}

		const TArray<TSharedPtr<FJsonValue>>* OutputArray = nullptr;
		if (!Root->TryGetArrayField(TEXT("output"), OutputArray) || OutputArray->Num() <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[OpenAI] output empty"));
			return false;
		}

		FString CombinedText;
		for (const TSharedPtr<FJsonValue>& OutputValue : *OutputArray)
		{
			const TSharedPtr<FJsonObject> OutputObject = OutputValue.IsValid() ? OutputValue->AsObject() : nullptr;
			if (!OutputObject.IsValid())
			{
				continue;
			}

			const TArray<TSharedPtr<FJsonValue>>* ContentArray = nullptr;
			if (!OutputObject->TryGetArrayField(TEXT("content"), ContentArray) || !ContentArray)
			{
				continue;
			}

			for (const TSharedPtr<FJsonValue>& ContentValue : *ContentArray)
			{
				const TSharedPtr<FJsonObject> ContentObject = ContentValue.IsValid() ? ContentValue->AsObject() : nullptr;
				if (!TryAppendTextField(ContentObject, CombinedText))
				{
					continue;
				}
			}
		}

		OutText = TrimAndStrip(CombinedText);
		if (OutText.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("[OpenAI] text not found in response body"));
			return false;
		}

		return true;
	}
}

FString UOpenAIRequester::BuildScatterPayloadJSON() const
{
	return BuildTextOnlyInputPayload(
		TEXT("gpt-4.1"),
		TEXT("You must output ONLY valid JSON with EXACT keys: scaleMin, scaleMax, rotationMin, rotationMax. No prose, no code fences."),
		60);
}

bool UOpenAIRequester::TryCreateAuthorizedRequest(const FString& Payload, TSharedRef<IHttpRequest, ESPMode::ThreadSafe>& OutRequest) const
{
	const FString ApiKey = FPlatformMisc::GetEnvironmentVariable(TEXT("OPENAI_API_KEY"));
	if (ApiKey.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[OpenAI] OPENAI_API_KEY environment variable is not set."));
		return false;
	}

	OutRequest->SetURL(TEXT("https://api.openai.com/v1/responses"));
	OutRequest->SetVerb(TEXT("POST"));
	OutRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	OutRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
	OutRequest->SetContentAsString(Payload);
	return true;
}

void UOpenAIRequester::SendOpenAIRequest()
{
	const FString Payload = BuildScatterPayloadJSON();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	if (!TryCreateAuthorizedRequest(Payload, Request))
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Sending Request: %s"), *Payload);

	Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bConnectedSuccessfully)
		{
			if (!bConnectedSuccessfully || !Res.IsValid())
			{
				UE_LOG(LogTemp, Error, TEXT("Accept Fail"));
				return;
			}

			UE_LOG(LogTemp, Log, TEXT("HTTP code: %d"), Res->GetResponseCode());
			UE_LOG(LogTemp, Log, TEXT("Answer: %s"), *Res->GetContentAsString());

			FString Text;
			if (!ExtractResponseText(Res->GetContentAsString(), Text))
			{
				return;
			}

			TSharedPtr<FJsonObject> Inner;
			if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Inner) || !Inner.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("[OpenAI] inner JSON parse failed: %s"), *Text);
				return;
			}

			FPCGScatterParams Out;
			double Num = 0.0;

			if (Inner->TryGetNumberField(TEXT("scaleMin"), Num))
			{
				Out.ScaleMin = static_cast<float>(Num);
			}
			if (Inner->TryGetNumberField(TEXT("scaleMax"), Num))
			{
				Out.ScaleMax = static_cast<float>(Num);
			}

			if (Inner->TryGetNumberField(TEXT("rotationMin"), Num))
			{
				Out.RotationMin = static_cast<float>(Num);
			}
			if (Inner->TryGetNumberField(TEXT("rotationMax"), Num))
			{
				Out.RotationMax = static_cast<float>(Num);
			}

			Out.ScaleMin = FMath::Clamp(Out.ScaleMin, 0.1f, 3.0f);
			Out.ScaleMax = FMath::Clamp(Out.ScaleMax, 0.1f, 3.0f);
			Out.RotationMin = FMath::Clamp(Out.RotationMin, -180.0f, 180.0f);
			Out.RotationMax = FMath::Clamp(Out.RotationMax, -180.0f, 180.0f);

			if (Out.ScaleMin > Out.ScaleMax)
			{
				Swap(Out.ScaleMin, Out.ScaleMax);
			}
			if (Out.RotationMin > Out.RotationMax)
			{
				Swap(Out.RotationMin, Out.RotationMax);
			}

			UE_LOG(LogTemp, Log, TEXT("[PCG:Test] scale(%.2f~%.2f), rot(%.1f~%.1f)"),
				Out.ScaleMin, Out.ScaleMax, Out.RotationMin, Out.RotationMax);

			OnScatterParams.Broadcast(Out);
		});

	Request->ProcessRequest();
}
