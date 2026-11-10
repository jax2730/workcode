//-----------------------------------------------------------------------------
// File:   EchoPCGSerializer.h
//
// Author: zhouguangyuan
//
// Date:   2025-9-25
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

#include "EchoComponentPrerequisites.h"

namespace Echo
{
	namespace PCGV2
	{
		class PCGObject;
		class PCGProperty;

		namespace PCGJsonSerializer
		{
			// Serialize all properties of the instance to jParent
			_EchoComponentExport cJSON* SerializePCGObject(const PCGObject* obj);

			// Deserialize all properties of the instance from jParent
			_EchoComponentExport bool Deserialize(PCGObject* obj, cJSON* jParent);

			// Create a json node for the property and its value, return the json node
			_EchoComponentExport cJSON* SerializeProperty(const PCGProperty* prop, const void* value);

			// Set the value from the json node to the property, return true if success
			_EchoComponentExport bool DeserializeProperty(cJSON* jNode, const PCGProperty* prop, void* value);

			_EchoComponentExport PCGObject* NewPCGObject(cJSON* jObject);
		}
	}
}
