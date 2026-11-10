//-----------------------------------------------------------------------------
// File:   EchoPCGSubgraph.cpp
//
// Author: zhouguangyuan
//
// Date:   2025-9-12
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "EchoComponentStableHeaders.h"
#include "PCG/Elements/EchoPCGSubgraph.h"

namespace Echo
{
	namespace PCGV2
	{
		PCGSubgraphSettings::PCGSubgraphSettings()
			: PCGSettings()
		{

		}

		PCGSubgraphSettings::~PCGSubgraphSettings()
		{

		}

		std::unique_ptr<PCGNode> PCGSubgraphSettings::CreateNode()
		{
			return std::make_unique<PCGSubgraphNode>();
		}

		PCGElement* PCGSubgraphSettings::GetElement() const
		{
			return PCGSubgraphElement::GetInstance();
		}

		PCGSettings* PCGSubgraphSettings::Clone() const
		{
			return CloneT<PCGSubgraphSettings>();;
		}

		REGISTER_PCG_CLASS(PCGSubgraphSettings)
		PCGClass* PCGSubgraphSettings::StaticClass()
		{
			BEGIN_PCG_CLASS(PCGSubgraphSettings)
			RegisterPCGSettings("PCGSubgraphSettings");
			END_PCG_CLASS()
		}

		PCGElement* PCGSubgraphElement::GetInstance()
		{
			static PCGSubgraphElement element;
			return &element;
		}

		PCGContext* PCGSubgraphElement::CreateContext()
		{
			return new PCGContext();
		}

		bool PCGSubgraphElement::ExecuteInternal(PCGContext* inContext)
		{
			return true;
		}

	}

}

