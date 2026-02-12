#include "AI/OpenAIRequester.h"
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
		const int32 First = S.Find(TEXT("\n"));
		const int32 Last = S.Find(TEXT("```"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);

        if (First != INDEX_NONE && Last != INDEX_NONE && Last > First) {
            S = S.Mid(First + 1, Last - (First + 1));
			S.TrimStartAndEndInline();
		}
    }

    return S;
}

namespace
{
    bool ExtractResponseText(const FString& Body, FString& OutText)
    {
		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body); 

        if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("[OpenAI] Top JSON parsing failed"));
            return false;
        }

        const TArray<TSharedPtr<FJsonValue>>* OutputArray = nullptr;
        if (!Root->TryGetArrayField(TEXT("output"), OutputArray) || OutputArray->Num() <= 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("[OpenAI] output empty"));
            return false;
        }

        const TSharedPtr<FJsonObject> Output0 = (*OutputArray)[0]->AsObject();
        if (!Output0.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("[OpenAI] output[0] parsing failed"));
            return false;
        }

        const TArray<TSharedPtr<FJsonValue>>* ContentArray = nullptr;
        if (!Output0->TryGetArrayField(TEXT("content"), ContentArray) || ContentArray->Num() <= 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("[OpenAI] content empty"));
            return false;
        }

        const TSharedPtr<FJsonObject> Content0 = (*ContentArray)[0]->AsObject();
        if (!Content0.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("[OpenAI] no text in content[0]"));
            return false;
        }

        OutText = TrimAndStrip(OutText);
        return true;
    }
}

FString UOpenAIRequester::BuildScatterPayloadJSON() const
{
    const FString Payload = R"({
        "model": "gpt-4.1",
        "input": "You must output ONLY valid JSON with EXACT keys: scaleMin, scaleMax, rotationMin, rotationMax. No prose, no code fences.",
        "max_output_tokens": 60
    })";
	return Payload;
}

FString UOpenAIRequester::BuildEvaluationPayloadJSON() const
{
    const FString Payload = R"({
        "model\": \"gpt-4.1",
        "input": \"Summarize the player's behavior as JSON with keys aggression, exploration, survival, support (0.0~1.0). Output only JSON.",
        "max_output_tokens": 120
    })";
    return Payload;
}


void UOpenAIRequester::SendOpenAIRequest()
{
	const FString Payload = BuildScatterPayloadJSON();

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

void UOpenAIRequester::SendPlayerEvaluationRequest()
{
    const FString Payload = BuildEvaluationPayloadJSON();

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL("https://api.openai.com/v1/responses");
    Request->SetVerb("POST");
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Authorization"), TEXT("Bearer Your-OpenAI Key"));
    Request->SetContentAsString(Payload);

    UE_LOG(LogTemp, Log, TEXT("Sending Evaluation Request: %s"), *Payload);

    Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bConnectedSuccessfully)
        {
            if (!bConnectedSuccessfully || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("Evaluation request failed"));
                return;
            }

            FString Text;
            if (!ExtractResponseText(Res->GetContentAsString(), Text))
            {
                return;
            }

            FPlayerEvaluationSnapshot Snapshot;
            if (!FPlayerEvaluationSnapshot::FromJson(Text, Snapshot))
            {
                UE_LOG(LogTemp, Warning, TEXT("[OpenAI] evaluation JSON parse failed: %s"), *Text);
                return;
            }

            UE_LOG(LogTemp, Log, TEXT("[BehaviorTree] Agg=%.2f Exp=%.2f Surv=%.2f Supp=%.2f"),
                Snapshot.AggressionScore, Snapshot.ExplorationScore, Snapshot.SurvivalScore, Snapshot.SupportScore);

            OnPlayerEvaluationReady.Broadcast(Snapshot);
        });

    Request->ProcessRequest();
}