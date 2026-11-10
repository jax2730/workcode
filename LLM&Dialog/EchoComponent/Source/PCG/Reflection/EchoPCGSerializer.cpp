//-----------------------------------------------------------------------------
// File:   EchoPCGSerializer.cpp
//
// Author: zhouguangyuan
//
// Date:   2025-9-25
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "PCG/Reflection/EchoPCGSerializer.h"

#include "PCG/Reflection/EchoPCGObjectV2.h"
#include "PCG/Reflection/EchoPCGClass.h"

namespace Echo
{
	namespace PCGV2
	{
		namespace PCGJsonSerializer
		{
			const char* PCGJsonSerializerKey_ClassName = "_ClassName";

			cJSON* SerializePCGObject(const PCGObject* obj)
			{
				cJSON* jObject = cJSON_CreateObject();

				if (!obj)
				{
					return jObject;
				}

				PCGClass* pcgClass = obj->GetClass();

				WriteJSNode(jObject, PCGJsonSerializerKey_ClassName, obj->GetClass()->GetName().c_str());

				const std::vector<PCGProperty*>& properties = pcgClass->GetProperties();
				for (const PCGProperty* prop : properties)
				{
					if (!prop->IsTransient())
					{
						const void* propAddress = prop->GetPropertyAddress(const_cast<PCGObject*>(obj));
						const char* propName = prop->GetName().c_str();
						cJSON* jPropNode = SerializeProperty(prop, propAddress);
						if (jPropNode)
						{
							cJSON_AddItemToObject(jObject, propName, jPropNode);
						}
					}
				}

				return jObject;
			}

			bool Deserialize(PCGObject* obj, cJSON* jParent)
			{
				if (!obj || !jParent)
				{
					return false;
				}

				PCGClass* meta = obj->GetClass();
				const std::vector<PCGProperty*>& properties = meta->GetProperties();
				for (const PCGProperty* prop : properties)
				{
					if (!prop->IsTransient())
					{
						void* address = prop->GetPropertyAddress(obj);
						JSNODE jPropNode = cJSON_GetObjectItem(jParent, prop->GetName().c_str());
						DeserializeProperty(jPropNode, prop, address);
					}
				}
				return true;
			}

			cJSON* CreateEnumJsonNode(const PCGEnumProperty* prop, const void* address)
			{
				Name enumValueName = prop->GetNameValue(address);
				return cJSON_CreateString(enumValueName.c_str());
			}

			cJSON* CreateVectorJsonNode(const float* values, int count)
			{
				cJSON* jNode = cJSON_CreateArray();
				for (int i = 0; i < count; i++)
				{
					cJSON_AddItemToArray(jNode, cJSON_CreateNumber(values[i]));
				}
				return jNode;
			}

			cJSON* CreateArrayJsonNode(const PCGArrayProperty* arrayProp, const void* arrayAddress)
			{
				cJSON* jNode = cJSON_CreateArray();
				const PCGProperty* elementProp = arrayProp->GetElementProperty();
				size_t count = arrayProp->GetArraySize(arrayAddress);
				if (elementProp)
				{
					for (int i = 0; i < count; i++)
					{
						const void* elementAddress = arrayProp->GetElementAddress(arrayAddress, i);
						cJSON* jElementNode = SerializeProperty(elementProp, elementAddress);
						if (jElementNode)
						{
							cJSON_AddItemToArray(jNode, jElementNode);
						}
					}
				}
				return jNode;
			}

			cJSON* CreateStructJsonNode(const PCGStructProperty* prop, const void* structAddress)
			{
				cJSON* jNode = cJSON_CreateObject();
				PCGClass* meta = prop->GetStructType();
				if (meta)
				{
					const std::vector<PCGProperty*>& properties = meta->GetProperties();
					for (const PCGProperty* subProp : properties)
					{
						const void* propAddress = subProp->GetPropertyAddress(const_cast<void*>(structAddress));
						cJSON* jSubNode = SerializeProperty(subProp, propAddress);
						if (jSubNode)
						{
							cJSON_AddItemToObject(jNode, subProp->GetName().c_str(), jSubNode);
						}
					}
				}
				return jNode;
			}

			cJSON* CreateObjectJsonNode(const PCGObjectProperty* prop, const void* address)
			{
				const PCGObject* obj = *(const PCGObject**)address;
				return SerializePCGObject(obj);
			}

			cJSON* CreateObjectJsonNode(const PCGSharedPtrProperty* prop, const void* address)
			{
				std::shared_ptr<const PCGObject> sharedPtr = prop->ToSharedPtr(address);
				const PCGObject* obj = sharedPtr.get();
				return SerializePCGObject(obj);
			}

			cJSON* SerializeProperty(const PCGProperty* prop, const void* address)
			{
				cJSON* jNode = nullptr;
				switch (prop->GetType())
				{
				case PCGPropertyType::ENUM:
					jNode = CreateEnumJsonNode(static_cast<const PCGEnumProperty*>(prop), address);
					break;
				case PCGPropertyType::BOOL:
					jNode = cJSON_CreateBool(*static_cast<const bool*>(address));
					break;
				case PCGPropertyType::INT8:
					jNode = cJSON_CreateNumber(*static_cast<const int8*>(address));
					break;
				case PCGPropertyType::INT16:
					jNode = cJSON_CreateNumber(*static_cast<const int16*>(address));
					break;
				case PCGPropertyType::INT:
					jNode = cJSON_CreateNumber(*static_cast<const int*>(address));
					break;
				case PCGPropertyType::INT64:
					// To avoid precision loss, serialize int64 as string
					jNode = cJSON_CreateString(std::to_string(*static_cast<const int64*>(address)).c_str());
					break;
				case PCGPropertyType::FLOAT:
					jNode = cJSON_CreateNumber(*static_cast<const float*>(address));
					break;
				case PCGPropertyType::DOUBLE:
					jNode = cJSON_CreateNumber(*static_cast<const double*>(address));
					break;
				case PCGPropertyType::STRING:
					jNode = cJSON_CreateString((*static_cast<const std::string*>(address)).c_str());
					break;
				case PCGPropertyType::NAME:
					jNode = cJSON_CreateString((*static_cast<const Name*>(address)).c_str());
					break;
				case PCGPropertyType::FILEPATH:
					jNode = cJSON_CreateString((*static_cast<const std::string*>(address)).c_str());
					break;
				case PCGPropertyType::VECTOR2:
					jNode = CreateVectorJsonNode(static_cast<const Vector2*>(address)->ptr(), 2);
					break;
				case PCGPropertyType::VECTOR3:
					jNode = CreateVectorJsonNode(static_cast<const Vector3*>(address)->ptr(), 3);
					break;
				case PCGPropertyType::VECTOR4:
					jNode = CreateVectorJsonNode(static_cast<const Vector4*>(address)->ptr(), 4);
					break;
				case PCGPropertyType::STRUCT:
					jNode = CreateStructJsonNode(static_cast<const PCGStructProperty*>(prop), address);
					break;
				case PCGPropertyType::OBJECT_PTR:
					jNode = CreateObjectJsonNode(static_cast<const PCGObjectProperty*>(prop), address);
					break;
				case PCGPropertyType::SHARED_PTR:
					jNode = CreateObjectJsonNode(static_cast<const PCGSharedPtrProperty*>(prop), address);
					break;
				case PCGPropertyType::ARRAY:
					jNode = CreateArrayJsonNode(static_cast<const PCGArrayProperty*>(prop), address);
					break;
				default:
					break;
				}
				return jNode;
			}

			bool DeserializeProperty(cJSON* jNode, const PCGEnumProperty* prop, void* address)
			{
				if (jNode && jNode->type == cJSON_String)
				{
					Name enumNameValue = Name(jNode->valuestring);
					prop->SetNameValue(address, enumNameValue);
					return true;
				}
				return false;
			}

			bool DeserializeProperty(cJSON* jNode, bool& value)
			{
				if (jNode && (jNode->type == cJSON_True || jNode->type == cJSON_False))
				{
					value = (jNode->type == cJSON_True);
					return true;
				}
				return false;
			}

			PCGObject* NewPCGObject(cJSON* jObject)
			{
				if (jObject && jObject->type == cJSON_Object)
				{
					JSNODE jClassNameNode = cJSON_GetObjectItem(jObject, PCGJsonSerializerKey_ClassName);
					if (jClassNameNode && jClassNameNode->type == cJSON_String)
					{
						Name className = Name(jClassNameNode->valuestring);
						PCGClass* pcgClass = PCGClassGlobals::GetClass(className);
						if (pcgClass)
						{
							PCGObject* newObj = NewPCGObject<PCGObject>(pcgClass);
							Deserialize(newObj, jObject);
							return newObj;
						}
					}
				}
				LogManager::instance()->logMessage("Failed to create PCGObject from JSON: invalid json node or class not found.");

				return nullptr;
			}

			template <typename IntegralType>
			bool DeserializeIntegralProperty(cJSON* jNode, IntegralType& value)
			{
				if (jNode && jNode->type == cJSON_Number)
				{
					value = static_cast<IntegralType>(jNode->valueint);
					return true;
				}
				return false;
			}

			// Specialization for int64
			template <>
			bool DeserializeIntegralProperty<int64>(cJSON* jNode, int64& value)
			{
				if (jNode && jNode->type == cJSON_String)
				{
					try
					{
						value = std::stoll(jNode->valuestring);
						return true;
					}
					catch (...)
					{
						return false;
					}
				}
				return false;
			}

			bool DeserializeProperty(cJSON* jNode, int8& value)
			{
				return DeserializeIntegralProperty(jNode, value);
			}

			bool DeserializeProperty(cJSON* jNode, int16& value)
			{
				return DeserializeIntegralProperty(jNode, value);
			}

			bool DeserializeProperty(cJSON* jNode, int32& value)
			{
				return DeserializeIntegralProperty(jNode, value);
			}

			bool DeserializeProperty(cJSON* jNode, int64& value)
			{
				return DeserializeIntegralProperty(jNode, value);
			}

			bool DeserializeProperty(cJSON* jNode, float& value)
			{
				if (jNode && jNode->type == cJSON_Number)
				{
					value = static_cast<float>(jNode->valuedouble);
					return true;
				}
				return false;
			}

			bool DeserializeProperty(cJSON* jNode, double& value)
			{
				if (jNode && jNode->type == cJSON_Number)
				{
					value = jNode->valuedouble;
					return true;
				}
				return false;
			}

			bool DeserializeProperty(cJSON* jNode, std::string& value)
			{
				if (jNode && jNode->type == cJSON_String)
				{
					value = jNode->valuestring;
					return true;
				}
				return false;
			}

			bool DeserializeProperty(cJSON* jNode, Name& value)
			{
				if (jNode && jNode->type == cJSON_String)
				{
					value = Name(jNode->valuestring);
					return true;
				}
				return false;
			}

			bool DeserializeProperty(cJSON* jNode, float* values, int count)
			{
				if (jNode && jNode->type == cJSON_Array)
				{
					int arraySize = cJSON_GetArraySize(jNode);
					if (arraySize == count)
					{
						for (int i = 0; i < count; i++)
						{
							cJSON* item = cJSON_GetArrayItem(jNode, i);
							if (item && item->type == cJSON_Number)
							{
								values[i] = static_cast<float>(item->valuedouble);
							}
							else
							{
								return false;
							}
						}
						return true;
					}
				}
				return false;
			}

			bool DeserializeProperty(cJSON* jNode, const PCGObjectProperty* prop, void* address)
			{
				if (jNode && jNode->type == cJSON_Object)
				{
					PCGObject* objectPtr = *static_cast<PCGObject**>(address);
					if (objectPtr)
					{
						Deserialize(objectPtr, jNode);
						return true;
					}
				}
				return false;
			}

			bool DeserializeProperty(cJSON* jNode, const PCGSharedPtrProperty* prop, void* address)
			{
				if (jNode && jNode->type == cJSON_Object)
				{
					std::shared_ptr<PCGObject> sharedPtr = prop->ToSharedPtr(address);
					PCGObject* objectPtr = sharedPtr.get();
					if (objectPtr)
					{
						Deserialize(objectPtr, jNode);
						return true;
					}
				}
				return false;
			}

			bool DeserializeProperty(cJSON* jNode, PCGArrayProperty* arrayProp, void* address)
			{
				if (jNode && jNode->type == cJSON_Array)
				{
					PCGProperty* elementProp = arrayProp->GetElementProperty();
					if (!elementProp)
						return false;

					int arraySize = cJSON_GetArraySize(jNode);
					arrayProp->ResizeArray(address, arraySize);
					for (int i = 0; i < arraySize; i++)
					{
						cJSON* jItemNode = cJSON_GetArrayItem(jNode, i);
						void* elementAddress = arrayProp->GetElementAddress(address, i);
						DeserializeProperty(jItemNode, elementProp, elementAddress);
					}
					return true;
				}
				return false;
			}

			bool DeserializeStructProperty(cJSON* jNode, const PCGStructProperty* prop, void* structAddress)
			{
				if (jNode && jNode->type == cJSON_Object)
				{
					PCGClass* meta = prop->GetStructType();
					if (meta)
					{
						const std::vector<PCGProperty*>& properties = meta->GetProperties();
						for (const PCGProperty* subProp : properties)
						{
							void* propAddress = subProp->GetPropertyAddress(structAddress);
							JSNODE jSubNode = cJSON_GetObjectItem(jNode, subProp->GetName().c_str());
							DeserializeProperty(jSubNode, subProp, propAddress);
						}
						return true;
					}
				}
				return false;
			}

			bool DeserializeProperty(cJSON* jNode, const PCGProperty* prop, void* address)
			{
				if (!jNode) return false;

				bool bSuccess = false;
				switch (prop->GetType())
				{
				case PCGPropertyType::ENUM:
					bSuccess = DeserializeProperty(jNode, static_cast<const PCGEnumProperty*>(prop), address);
					break;
				case PCGPropertyType::BOOL:
					bSuccess = DeserializeProperty(jNode, *static_cast<bool*>(address));
					break;
				case PCGPropertyType::INT8:
					bSuccess = DeserializeProperty(jNode, *static_cast<int8*>(address));
					break;
				case PCGPropertyType::INT:
					bSuccess = DeserializeProperty(jNode, *static_cast<int*>(address));
					break;
				case PCGPropertyType::INT64:
					bSuccess = DeserializeProperty(jNode, *static_cast<int64*>(address));
					break;
				case PCGPropertyType::FLOAT:
					bSuccess = DeserializeProperty(jNode, *static_cast<float*>(address));
					break;
				case PCGPropertyType::DOUBLE:
					bSuccess = DeserializeProperty(jNode, *static_cast<double*>(address));
					break;
				case PCGPropertyType::STRING:
					bSuccess = DeserializeProperty(jNode, *static_cast<std::string*>(address));
					break;
				case PCGPropertyType::NAME:
					bSuccess = DeserializeProperty(jNode, *static_cast<Name*>(address));
					break;
				case PCGPropertyType::FILEPATH:
					bSuccess = DeserializeProperty(jNode, *static_cast<std::string*>(address));
					break;
				case PCGPropertyType::VECTOR2:
					bSuccess = DeserializeProperty(jNode, static_cast<float*>(static_cast<Vector2*>(address)->ptr()), 2);
					break;
				case PCGPropertyType::VECTOR3:
					bSuccess = DeserializeProperty(jNode, static_cast<float*>(static_cast<Vector3*>(address)->ptr()), 3);
					break;
				case PCGPropertyType::VECTOR4:
					bSuccess = DeserializeProperty(jNode, static_cast<float*>(static_cast<Vector4*>(address)->ptr()), 4);
					break;
				case PCGPropertyType::STRUCT:
					bSuccess = DeserializeStructProperty(jNode, static_cast<const PCGStructProperty*>(prop), address);
					break;
				case PCGPropertyType::OBJECT_PTR:
					bSuccess = DeserializeProperty(jNode, static_cast<const PCGObjectProperty*>(prop), address);
					break;
				case PCGPropertyType::SHARED_PTR:
					bSuccess = DeserializeProperty(jNode, static_cast<const PCGSharedPtrProperty*>(prop), address);
					break;
				case PCGPropertyType::ARRAY:
					bSuccess = DeserializeProperty(jNode, static_cast<PCGArrayProperty*>(const_cast<PCGProperty*>(prop)), address);
					break;
				default:
					break;
				}
				return bSuccess;
			}
		}
	}
}
