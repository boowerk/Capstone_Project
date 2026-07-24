#include "VFX/GP_DeathVFXSetupLibrary.h"

#include "NiagaraCommon.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraSystem.h"

bool UGP_DeathVFXSetupLibrary::ConfigureAbsorptionMaterialBinding(
	UNiagaraSystem* NiagaraSystem,
	FName MaterialParameterName)
{
#if WITH_EDITOR
	if (!IsValid(NiagaraSystem) || MaterialParameterName.IsNone())
	{
		return false;
	}

	const FNiagaraVariable MaterialParameter(
		FNiagaraTypeDefinition::GetUMaterialDef(),
		MaterialParameterName);

	NiagaraSystem->Modify();
	FNiagaraUserRedirectionParameterStore& ExposedParameters =
		NiagaraSystem->GetExposedParameters();
	if (ExposedParameters.IndexOf(MaterialParameter) == INDEX_NONE)
	{
		ExposedParameters.AddParameter(MaterialParameter);
	}

	bool bFoundSpriteRenderer = false;
	for (FNiagaraEmitterHandle& EmitterHandle : NiagaraSystem->GetEmitterHandles())
	{
		FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
		if (!EmitterData)
		{
			continue;
		}

		for (UNiagaraRendererProperties* RendererProperties : EmitterData->GetRenderers())
		{
			UNiagaraSpriteRendererProperties* SpriteRenderer =
				Cast<UNiagaraSpriteRendererProperties>(RendererProperties);
			if (!IsValid(SpriteRenderer))
			{
				continue;
			}

			bFoundSpriteRenderer = true;
			if (SpriteRenderer->MaterialUserParamBinding.Parameter != MaterialParameter)
			{
				SpriteRenderer->Modify();
				SpriteRenderer->MaterialUserParamBinding =
					FNiagaraUserParameterBinding(FNiagaraTypeDefinition::GetUMaterialDef());
				SpriteRenderer->MaterialUserParamBinding.Parameter = MaterialParameter;
			}
		}
	}

	if (!bFoundSpriteRenderer)
	{
		return false;
	}

	NiagaraSystem->RequestCompile(true);
	NiagaraSystem->PollForCompilationComplete();
	NiagaraSystem->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}
