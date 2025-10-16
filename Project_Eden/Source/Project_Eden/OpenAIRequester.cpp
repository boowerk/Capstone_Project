#include "OpenAIRequester.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

void UOpenAIRequester::SendOpenAIRequest()
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    Request->SetURL("https://api.openai.com/v1/responses");
    Request->SetVerb("POST");

    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Authorization"), TEXT("Bearer Your-OpenAI Key"));

    FString Payload = R"({
        "model": "gpt-4.1",
        "input": "Hello I'm Testing",
        "max_output_tokens": 100
    })";

    Request->SetContentAsString(Payload);

    Request->OnProcessRequestComplete().BindLambda([](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bConnectedSuccessfully)
        {
            if (!bConnectedSuccessfully || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("Accept Fail"));
                return;
            }

            UE_LOG(LogTemp, Log, TEXT("HTTP code: %d"), Res->GetResponseCode());
            UE_LOG(LogTemp, Log, TEXT("Answer: % s"), *Res->GetContentAsString());
        });

    Request->ProcessRequest();
}