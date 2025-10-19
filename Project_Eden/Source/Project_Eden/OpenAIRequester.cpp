#include "OpenAIRequester.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

static FString TrimAndStrip(const FString& Input)
{
	FString S = Input;
	S.TrimStartAndEndInline();
    if (S.StartsWith(TEXT("```"))) {
        int32 First = S.Find(TEXT("\n"));
		int32 Last = S.Find(TEXT("```"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);

        if (First != INDEX_NONE && Last != INDEX_NONE && Last > First) {
            S = S.Mid(First + 1, Last - (First + 1));
			S.TrimStartAndEndInline();
		}
    }

    return S;
}

FString UOpenAIRequester::BuildPayloadJSON() const
{
    const FString Payload = R"({
        "model": "gpt-4.1",
        "input": "You must output ONLY valid JSON with EXACT keys: scaleMin, scaleMax, rotationMin, rotationMax. No prose, no code fences.",
        "max_output_tokens": 60
    })";
	return Payload;
}

void UOpenAIRequester::SendOpenAIRequest()
{
	const FString Payload = BuildPayloadJSON();

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL("https://api.openai.com/v1/responses");
    Request->SetVerb("POST");
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Authorization"), TEXT("Bearer Your-OpenAI Key"));
    Request->SetContentAsString(Payload);

	UE_LOG(LogTemp, Log, TEXT("Sending Request: %s"), *Payload);

    Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bConnectedSuccessfully)
        {
            if (!bConnectedSuccessfully || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("Accept Fail"));
                return;
            }

            UE_LOG(LogTemp, Log, TEXT("HTTP code: %d"), Res->GetResponseCode());
            UE_LOG(LogTemp, Log, TEXT("Answer: % s"), *Res->GetContentAsString());

			// chat/completions response parsing
            TSharedPtr<FJsonObject> Root;
            const FString Body = Res->GetContentAsString();
            const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
            if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("[OpenAI] Top JSON parsing failed"));
                return;
            }

            // 2) output[0]
            const TArray<TSharedPtr<FJsonValue>>* OutputArray = nullptr;
            if (!Root->TryGetArrayField(TEXT("output"), OutputArray) || OutputArray->Num() <= 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("[OpenAI] output empty"));
                return;
            }
            const TSharedPtr<FJsonObject> Output0 = (*OutputArray)[0]->AsObject();
            if (!Output0.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("[OpenAI] output[0] parsing failed"));
                return;
            }

            // 3) output[0].content[0]
            const TArray<TSharedPtr<FJsonValue>>* ContentArray = nullptr;
            if (!Output0->TryGetArrayField(TEXT("content"), ContentArray) || ContentArray->Num() <= 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("[OpenAI] content empty"));
                return;
            }
            const TSharedPtr<FJsonObject> Content0 = (*ContentArray)[0]->AsObject();
            if (!Content0.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("[OpenAI] content[0] parsing failed"));
                return;
            }

            // 4) content[0].text  →  JSON 문자열(개행/들여쓰기 포함)
            FString Text;
            if (!Content0->TryGetStringField(TEXT("text"), Text))
            {
                UE_LOG(LogTemp, Warning, TEXT("[OpenAI] no text in content[0]"));
                return;
            }

            // (선택) 코드펜스 제거/트림
            Text.TrimStartAndEndInline();
            if (Text.StartsWith(TEXT("```")))
            {
                const int32 First = Text.Find(TEXT("\n"));
                const int32 Last = Text.Find(TEXT("```"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
                if (First != INDEX_NONE && Last != INDEX_NONE && Last > First)
                    Text = Text.Mid(First + 1, Last - (First + 1)).TrimStartAndEnd();
            }

            // 5) 내부 JSON 파싱 (우리가 원하는 4개 값)
            TSharedPtr<FJsonObject> Inner;
            if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Inner) || !Inner.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("[OpenAI] inner JSON parse failed: %s"), *Text);
                return;
            }

            FPCGScatterParams Out;
            double Num = 0.0;

            if (Inner->TryGetNumberField(TEXT("scaleMin"), Num))     Out.ScaleMin = (float)Num;
            if (Inner->TryGetNumberField(TEXT("scaleMax"), Num))     Out.ScaleMax = (float)Num;
            if (Inner->TryGetNumberField(TEXT("rotationMin"), Num))  Out.RotationMin = (float)Num;
            if (Inner->TryGetNumberField(TEXT("rotationMax"), Num))  Out.RotationMax = (float)Num;

            Out.ScaleMin = FMath::Clamp(Out.ScaleMin, 0.1f, 3.0f);
            Out.ScaleMax = FMath::Clamp(Out.ScaleMax, 0.1f, 3.0f);
            Out.RotationMin = FMath::Clamp(Out.RotationMin, -180.0f, 180.0f);
            Out.RotationMax = FMath::Clamp(Out.RotationMax, -180.0f, 180.0f);

            if (Out.ScaleMin > Out.ScaleMax) Swap(Out.ScaleMin, Out.ScaleMax);
            if (Out.RotationMin > Out.RotationMax) Swap(Out.RotationMin, Out.RotationMax);

            UE_LOG(LogTemp, Log, TEXT("[PCG:Test] scale(%.2f~%.2f), rot(%.1f~%.1f)"),
                Out.ScaleMin, Out.ScaleMax, Out.RotationMin, Out.RotationMax);

            OnScatterParams.Broadcast(Out);

        });

    Request->ProcessRequest();
}
