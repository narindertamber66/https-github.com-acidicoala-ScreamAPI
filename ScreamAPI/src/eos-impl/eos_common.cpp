#include "pch.h"
#include "eos-sdk/eos_common.h"
#include <ScreamAPI.h>

EOS_DECLARE_FUNC(const char*) EOS_EResult_ToString(EOS_EResult Result){
	static auto proxy = ScreamAPI::proxyFunction(&EOS_EResult_ToString, __func__);
	return proxy(Result);
}

EOS_DECLARE_FUNC(EOS_Bool) EOS_EpicAccountId_IsValid(EOS_EpicAccountId AccountId){
	static auto proxy = ScreamAPI::proxyFunction(&EOS_EpicAccountId_IsValid, __func__);
	return proxy(AccountId);
}