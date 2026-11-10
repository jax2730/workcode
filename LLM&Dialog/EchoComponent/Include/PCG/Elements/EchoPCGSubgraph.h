//-----------------------------------------------------------------------------
// File:   EchoPCGSubgraph.h
//
// Author: zhouguangyuan
//
// Date:   2025-9-12
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

#include "EchoComponentPrerequisites.h"

#include "PCG/EchoPCGSettings.h"
#include "PCG/EchoPCGElement.h"
#include "PCG/EchoPCGContext.h"
#include "PCG/EchoPCGGraph.h"

namespace Echo
{
	namespace PCGV2
	{
		class PCGSubgraphSettings : public PCGSettings
		{
		public:
			PCGSubgraphSettings();
			virtual ~PCGSubgraphSettings();

			PCG_OBJECT_BODY(PCGSubgraphSettings)

			virtual std::unique_ptr<PCGNode> CreateNode() override;

			virtual PCGElement* GetElement() const override;

			virtual Name GetDefaultNodeTitle() const override { return Name("SubGraph"); }

			virtual PCGSettings* Clone() const override;

			PCGGraphPtr m_Graph;
		};

		class PCGSubgraphNode : public PCGNode
		{
			PCGGraph* GetSubgraph() { return static_cast<PCGSubgraphSettings*>(GetSettings())->m_Graph.get(); }
		};

		class PCGSubgraphElement : public PCGElement
		{
		public:
			static PCGElement* GetInstance();
		protected:
			virtual PCGContext* CreateContext() override;

			bool ExecuteInternal(PCGContext* inContext) override;
		};
	}
}
