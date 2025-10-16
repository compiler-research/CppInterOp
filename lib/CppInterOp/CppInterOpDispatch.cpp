//------------------------------------------------------------------------------
// CLING - the C++ LLVM-based InterpreterG :)
// author:  Vassil Vassilev <vasil.georgiev.vasilev@cern.ch>
// author:  Aaron Jomy <aaron.jomy@cern.ch>
//
// This file is dual-licensed: you can choose to license it under the University
// of Illinois Open Source License or the GNU Lesser General Public License. See
// LICENSE.TXT for details.
//------------------------------------------------------------------------------

#include <CppInterOp/CppInterOpDispatch.h>

#include <unordered_map>

static const FunctionEntry api_function_table[] = {
    {"GetInterpreter", (__CPP_FUNC)Cpp::GetInterpreter},
    {"CreateInterpreter", (__CPP_FUNC)Cpp::CreateInterpreter},
    {"Process", (__CPP_FUNC)Cpp::Process},
    {"GetResourceDir", (__CPP_FUNC)Cpp::GetResourceDir},
    {"AddIncludePath", (__CPP_FUNC)Cpp::AddIncludePath},
    {"LoadLibrary", (__CPP_FUNC)Cpp::LoadLibrary},
    {"Declare", (__CPP_FUNC)Cpp::Declare},
    {"DeleteInterpreter", (__CPP_FUNC)Cpp::DeleteInterpreter},
    {"IsNamespace", (__CPP_FUNC)Cpp::IsNamespace},
    {"ObjToString", (__CPP_FUNC)Cpp::ObjToString},
    {"GetQualifiedCompleteName", (__CPP_FUNC)Cpp::GetQualifiedCompleteName},
    {"IsLValueReferenceType", (__CPP_FUNC)Cpp::IsLValueReferenceType},
    {"GetNonReferenceType", (__CPP_FUNC)Cpp::GetNonReferenceType},
    {"IsEnumType", (__CPP_FUNC)Cpp::IsEnumType},
    {"GetIntegerTypeFromEnumType", (__CPP_FUNC)Cpp::GetIntegerTypeFromEnumType},
    {"GetReferencedType", (__CPP_FUNC)Cpp::GetReferencedType},
    {"IsPointerType", (__CPP_FUNC)Cpp::IsPointerType},
    {"GetPointeeType", (__CPP_FUNC)Cpp::GetPointeeType},
    {"GetPointerType", (__CPP_FUNC)Cpp::GetPointerType},
    {"IsReferenceType", (__CPP_FUNC)Cpp::IsReferenceType},
    {"GetTypeAsString", (__CPP_FUNC)Cpp::GetTypeAsString},
    {"GetCanonicalType", (__CPP_FUNC)Cpp::GetCanonicalType},
    {"HasTypeQualifier", (__CPP_FUNC)Cpp::HasTypeQualifier},
    {"RemoveTypeQualifier", (__CPP_FUNC)Cpp::RemoveTypeQualifier},
    {"GetUnderlyingType", (__CPP_FUNC)Cpp::GetUnderlyingType},
    {"IsRecordType", (__CPP_FUNC)Cpp::IsRecordType},
    {"IsFunctionPointerType", (__CPP_FUNC)Cpp::IsFunctionPointerType},
    {"GetVariableType", (__CPP_FUNC)Cpp::GetVariableType},
    {"GetNamed", (__CPP_FUNC)Cpp::GetNamed},
    {"GetScopeFromType", (__CPP_FUNC)Cpp::GetScopeFromType},
    {"GetClassTemplateInstantiationArgs",
     (__CPP_FUNC)Cpp::GetClassTemplateInstantiationArgs},
    {"IsClass", (__CPP_FUNC)Cpp::IsClass},
    {"GetType", (__CPP_FUNC)Cpp::GetType},
    {"GetTypeFromScope", (__CPP_FUNC)Cpp::GetTypeFromScope},
    {"GetComplexType", (__CPP_FUNC)Cpp::GetComplexType},
    {"GetIntegerTypeFromEnumScope",
     (__CPP_FUNC)Cpp::GetIntegerTypeFromEnumScope},
    {"GetUnderlyingScope", (__CPP_FUNC)Cpp::GetUnderlyingScope},
    {"GetScope", (__CPP_FUNC)Cpp::GetScope},
    {"GetGlobalScope", (__CPP_FUNC)Cpp::GetGlobalScope},
    {"GetScopeFromCompleteName", (__CPP_FUNC)Cpp::GetScopeFromCompleteName},
    {"InstantiateTemplate", (__CPP_FUNC)Cpp::InstantiateTemplate},
    {"GetParentScope", (__CPP_FUNC)Cpp::GetParentScope},
    {"IsTemplate", (__CPP_FUNC)Cpp::IsTemplate},
    {"IsTemplateSpecialization", (__CPP_FUNC)Cpp::IsTemplateSpecialization},
    {"IsTypedefed", (__CPP_FUNC)Cpp::IsTypedefed},
    {"IsClassPolymorphic", (__CPP_FUNC)Cpp::IsClassPolymorphic},
    {"Demangle", (__CPP_FUNC)Cpp::Demangle},
    {"SizeOf", (__CPP_FUNC)Cpp::SizeOf},
    {"GetSizeOfType", (__CPP_FUNC)Cpp::GetSizeOfType},
    {"IsBuiltin", (__CPP_FUNC)Cpp::IsBuiltin},
    {"IsComplete", (__CPP_FUNC)Cpp::IsComplete},
    {"Allocate", (__CPP_FUNC)Cpp::Allocate},
    {"Deallocate", (__CPP_FUNC)Cpp::Deallocate},
    {"Construct", (__CPP_FUNC)Cpp::Construct},
    {"Destruct", (__CPP_FUNC)Cpp::Destruct},
    {"MakeFunctionCallable",
     (__CPP_FUNC) static_cast<Cpp::JitCall (*)(Cpp::TCppConstFunction_t)>(
         &Cpp::MakeFunctionCallable)},
    {"GetFunctionAddress",
     (__CPP_FUNC) static_cast<Cpp::TCppFuncAddr_t (*)(Cpp::TCppFunction_t)>(
         &Cpp::GetFunctionAddress)},
    {"IsAbstract", (__CPP_FUNC)Cpp::IsAbstract},
    {"IsEnumScope", (__CPP_FUNC)Cpp::IsEnumScope},
    {"IsEnumConstant", (__CPP_FUNC)Cpp::IsEnumConstant},
    {"IsAggregate", (__CPP_FUNC)Cpp::IsAggregate},
    {"HasDefaultConstructor", (__CPP_FUNC)Cpp::HasDefaultConstructor},
    {"IsVariable", (__CPP_FUNC)Cpp::IsVariable},
    {"GetAllCppNames", (__CPP_FUNC)Cpp::GetAllCppNames},
    {"GetUsingNamespaces", (__CPP_FUNC)Cpp::GetUsingNamespaces},
    {"GetCompleteName", (__CPP_FUNC)Cpp::GetCompleteName},
    {"GetDestructor", (__CPP_FUNC)Cpp::GetDestructor},
    {"IsVirtualMethod", (__CPP_FUNC)Cpp::IsVirtualMethod},
    {"GetNumBases", (__CPP_FUNC)Cpp::GetNumBases},
    {"GetName", (__CPP_FUNC)Cpp::GetName},
    {"GetBaseClass", (__CPP_FUNC)Cpp::GetBaseClass},
    {"IsSubclass", (__CPP_FUNC)Cpp::IsSubclass},
    {"GetOperator", (__CPP_FUNC)Cpp::GetOperator},
    {"GetFunctionReturnType", (__CPP_FUNC)Cpp::GetFunctionReturnType},
    {"GetBaseClassOffset", (__CPP_FUNC)Cpp::GetBaseClassOffset},
    {"GetClassMethods", (__CPP_FUNC)Cpp::GetClassMethods},
    {"GetFunctionsUsingName", (__CPP_FUNC)Cpp::GetFunctionsUsingName},
    {"GetFunctionNumArgs", (__CPP_FUNC)Cpp::GetFunctionNumArgs},
    {"GetFunctionRequiredArgs", (__CPP_FUNC)Cpp::GetFunctionRequiredArgs},
    {"GetFunctionArgName", (__CPP_FUNC)Cpp::GetFunctionArgName},
    {"GetFunctionArgType", (__CPP_FUNC)Cpp::GetFunctionArgType},
    {"GetFunctionArgDefault", (__CPP_FUNC)Cpp::GetFunctionArgDefault},
    {"IsConstMethod", (__CPP_FUNC)Cpp::IsConstMethod},
    {"GetFunctionTemplatedDecls", (__CPP_FUNC)Cpp::GetFunctionTemplatedDecls},
    {"ExistsFunctionTemplate", (__CPP_FUNC)Cpp::ExistsFunctionTemplate},
    {"IsTemplatedFunction", (__CPP_FUNC)Cpp::IsTemplatedFunction},
    {"IsStaticMethod", (__CPP_FUNC)Cpp::IsStaticMethod},
    {"GetClassTemplatedMethods", (__CPP_FUNC)Cpp::GetClassTemplatedMethods},
    {"BestOverloadFunctionMatch", (__CPP_FUNC)Cpp::BestOverloadFunctionMatch},
    {"GetOperatorFromSpelling", (__CPP_FUNC)Cpp::GetOperatorFromSpelling},
    {"IsFunctionDeleted", (__CPP_FUNC)Cpp::IsFunctionDeleted},
    {"IsPublicMethod", (__CPP_FUNC)Cpp::IsPublicMethod},
    {"IsProtectedMethod", (__CPP_FUNC)Cpp::IsProtectedMethod},
    {"IsPrivateMethod", (__CPP_FUNC)Cpp::IsPrivateMethod},
    {"IsConstructor", (__CPP_FUNC)Cpp::IsConstructor},
    {"IsDestructor", (__CPP_FUNC)Cpp::IsDestructor},
    {"GetDatamembers", (__CPP_FUNC)Cpp::GetDatamembers},
    {"GetStaticDatamembers", (__CPP_FUNC)Cpp::GetStaticDatamembers},
    {"GetEnumConstantDatamembers", (__CPP_FUNC)Cpp::GetEnumConstantDatamembers},
    {"LookupDatamember", (__CPP_FUNC)Cpp::LookupDatamember},
    {"IsLambdaClass", (__CPP_FUNC)Cpp::IsLambdaClass},
    {"GetQualifiedName", (__CPP_FUNC)Cpp::GetQualifiedName},
    {"GetVariableOffset", (__CPP_FUNC)Cpp::GetVariableOffset},
    {"IsPublicVariable", (__CPP_FUNC)Cpp::IsPublicVariable},
    {"IsProtectedVariable", (__CPP_FUNC)Cpp::IsProtectedVariable},
    {"IsPrivateVariable", (__CPP_FUNC)Cpp::IsPrivateVariable},
    {"IsStaticVariable", (__CPP_FUNC)Cpp::IsStaticVariable},
    {"IsConstVariable", (__CPP_FUNC)Cpp::IsConstVariable},
    {"GetDimensions", (__CPP_FUNC)Cpp::GetDimensions},
    {"GetEnumConstants", (__CPP_FUNC)Cpp::GetEnumConstants},
    {"GetEnumConstantType", (__CPP_FUNC)Cpp::GetEnumConstantType},
    {"GetEnumConstantValue", (__CPP_FUNC)Cpp::GetEnumConstantValue},
    {"DumpScope", (__CPP_FUNC)Cpp::DumpScope},
    {"AddSearchPath", (__CPP_FUNC)Cpp::AddSearchPath},
    {"Evaluate", (__CPP_FUNC)Cpp::Evaluate},
    {"IsDebugOutputEnabled", (__CPP_FUNC)Cpp::IsDebugOutputEnabled},
    {"EnableDebugOutput", (__CPP_FUNC)Cpp::EnableDebugOutput},
    {nullptr, nullptr} /* end of list */
};

static inline __CPP_FUNC _cppinterop_get_proc_address(const char* funcName) {
  static const std::unordered_map<std::string_view, __CPP_FUNC> function_map =
      [] {
        std::unordered_map<std::string_view, __CPP_FUNC> map;
        size_t count = 0;
        while (api_function_table[count].name != nullptr)
          ++count;
        map.reserve(count);

        for (size_t i = 0; i < count; ++i) {
          map[api_function_table[i].name] = api_function_table[i].address;
        }
        return map;
      }();

  auto it = function_map.find(funcName);
  return (it != function_map.end()) ? it->second : nullptr;
}

void (*CppGetProcAddress(const unsigned char* procName))(void) {
  return _cppinterop_get_proc_address(reinterpret_cast<const char*>(procName));
}
