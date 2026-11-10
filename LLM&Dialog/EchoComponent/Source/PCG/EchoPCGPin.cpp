#include "EchoComponentStableHeaders.h"
#include "PCG/EchoPCGPin.h"

#include "PCG/EchoPCGGraph.h"

namespace Echo
{
	namespace PCGV2
	{
		PCGPinProperties::PCGPinProperties(const Name& inLabel, PCGDataType InAllowedTypes /*= PCGDataType::Any*/, bool bInAllowMultipleConnections /*= true*/, bool bAllowMultipleData /*= true*/, const String& inTooltip /*= ""*/)
			: label(inLabel)
			, allowedTypes(InAllowedTypes)
			, bAllowsMultipleData(bAllowMultipleData)
			, tooltip(inTooltip)
			, bAllowsMultipleConnections(bInAllowMultipleConnections)
		{

		}

		PCGClass* PCGPinProperties::StaticClass()
		{
			BEGIN_PCG_CLASS(PCGPinProperties)

				Class.AddProperty("Label", PCGPropertyType::NAME, STRUCT_OFFSET(PCGPinProperties, label));
				Class.AddEnumProperty("AllowedTypes", STRUCT_OFFSET(PCGPinProperties, allowedTypes), StaticEnum<PCGDataType>());
				Class.AddProperty("Tooltip", PCGPropertyType::STRING, STRUCT_OFFSET(PCGPinProperties, tooltip));

			END_PCG_CLASS()
		}

		bool PCGPinProperties::operator==(const PCGPinProperties& other) const
		{
			return label == other.label && allowedTypes == other.allowedTypes;
		}

		void PCGPinProperties::SetAllowMultipleConnections(bool bInAllowsMultipleConnections)
		{
			if (bAllowsMultipleConnections)
			{
				bAllowsMultipleData = true;
			}
			bAllowsMultipleConnections = bInAllowsMultipleConnections;
		}

		uint32 PCGPinProperties::GetTypeHash(const PCGPinProperties& Value)
		{
			uint32 Hash = Value.label.getHashVal();
			Hash = HashCombine(Hash, static_cast<uint32>(Value.allowedTypes));
			Hash = HashCombine(Hash, Value.bAllowsMultipleConnections);
			Hash = HashCombine(Hash, Value.bAllowsMultipleData);
			Hash = HashCombine(Hash, static_cast<uint32>(Value.usage));
			Hash = HashCombine(Hash, static_cast<uint32>(Value.pinStatus));
			Hash = HashCombine(Hash, Value.bInvisiblePin);

			return Hash;
 		}


		PCGPin::PCGPin(PCGNode* node)
			: m_Node(node)
		{

		}

		PCGPin::~PCGPin()
		{

		}

		bool PCGPin::IsOutputPin() const
		{
			if (!m_Node)
			{
				return false;
			}
			return m_Node->GetOutputPin(m_Properties.label) == this;
		}

		bool PCGPin::AddEdgeTo(PCGPin* otherPin, std::set<PCGNode*>* outTouchedNodes /*= nullptr*/)
		{
			if (otherPin == nullptr || otherPin == this)
			{
				return false;
			}

			const bool bThisPinIsUpstream = IsOutputPin();
			const bool bOtherPinIsUpstream = otherPin->IsOutputPin();

			if (bThisPinIsUpstream == bOtherPinIsUpstream)
			{
				return false;
			}

			PCGEdgePtr newEdge = std::make_shared<PCGEdge>();
			m_Edges.push_back(newEdge);
			otherPin->m_Edges.push_back(newEdge);

			newEdge->m_InputPin = bThisPinIsUpstream ? this : otherPin;
			newEdge->m_OutputPin = bThisPinIsUpstream ? otherPin : this;

			if (outTouchedNodes)
			{
				outTouchedNodes->insert(m_Node);
				outTouchedNodes->insert(otherPin->m_Node);
			}

			return true;
		}

		bool PCGPin::BreakEdgeTo(PCGPin* otherPin, std::set<PCGNode*>* outTouchedNodes /*= nullptr*/)
		{
			if (!otherPin)
			{
				return false;
			}

			for (auto& edge : m_Edges)
			{
				if (edge->GetOtherPin(this) == otherPin)
				{
					otherPin->m_Edges.erase(std::remove(m_Edges.begin(), m_Edges.end(), edge), m_Edges.end());

					if (outTouchedNodes)
					{
						outTouchedNodes->insert(m_Node);
						outTouchedNodes->insert(otherPin->m_Node);
					}
					return true;
				}
			}

			return false;
		}

		bool PCGPin::BreakAllEdges(std::set<PCGNode*>* outTouchedNodes /*= nullptr*/)
		{
			bool bChanged = false;

			if (!m_Edges.empty())
			{
				if (outTouchedNodes)
				{
					outTouchedNodes->insert(m_Node);
				}
			}

			for (auto& edge : m_Edges)
			{
				PCGPin* otherPin = edge->GetOtherPin(this);
				if (otherPin)
				{
					std::vector<PCGEdgePtr>& otherPinEdges = otherPin->m_Edges;
					otherPinEdges.erase(std::remove(otherPinEdges.begin(), otherPinEdges.end(), edge), otherPinEdges.end());
					bChanged = true;
					if (outTouchedNodes)
					{
						outTouchedNodes->insert(otherPin->m_Node);
					}
				}
			}

			return bChanged;
		}

		bool PCGPin::BreakAllIncompatibleEdges(std::set<PCGNode*>* outTouchedNodes)
		{
			bool bChanged = false;
			bool bHasAValidEdge = false;

			for (int32 edgeIndex = static_cast<int32>(m_Edges.size()) - 1; edgeIndex >= 0; --edgeIndex)
			{
				PCGEdgePtr& edge = m_Edges[edgeIndex];
				PCGPin* otherPin = edge->GetOtherPin(this);

				bool bRemoveEdge = !IsCompatibleWith(otherPin) || (!AllowsMultipleConnections() && bHasAValidEdge);

				if (bRemoveEdge)
				{
					m_Edges.erase(m_Edges.begin() + edgeIndex);

					if (outTouchedNodes)
					{
						outTouchedNodes->insert(m_Node);
					}
					if (otherPin)
					{
						otherPin->m_Edges.erase(std::remove(otherPin->m_Edges.begin(), otherPin->m_Edges.end(), edge), otherPin->m_Edges.end());
						bChanged = true;

						if (outTouchedNodes)
						{
							outTouchedNodes->insert(otherPin->m_Node);
						}
					}
				}
				else
				{
					bHasAValidEdge = true;
				}
			}
			return bChanged;
		}

		bool SortPinsAndRetrieveTypes(const PCGPin* inPinA, const PCGPin* inPinB, const PCGPin*& outUpstreamPin, const PCGPin*& outDownstreamPin, PCGDataType& outUpstreamTypes, PCGDataType& outDownstreamTypes)
		{
			assert(inPinA && inPinB);

			const bool bPinAIsOutput = inPinA->IsOutputPin();

			// Cannot connect two pins of same polarity
			if (bPinAIsOutput == inPinB->IsOutputPin())
			{
				return false;
			}

			outUpstreamPin = bPinAIsOutput ? inPinA : inPinB;
			outDownstreamPin = bPinAIsOutput ? inPinB : inPinA;

			assert(outUpstreamPin && outDownstreamPin);
			outUpstreamTypes = outUpstreamPin->GetCurrentTypes();
			outDownstreamTypes = outDownstreamPin->m_Properties.allowedTypes;

			return true;
		}

		bool PCGPin::IsConnected() const
		{
			for (const PCGEdgePtr& Edge : m_Edges)
			{
				if (Edge->IsValid())
				{
					return true;
				}
			}

			return false;
 		}

		bool PCGPin::IsCompatibleWith(PCGPin* otherPin) const
		{
			if (!otherPin)
			{
				return false;
			}

			const PCGPin* upstreamPin = nullptr;
			const PCGPin* downstreamPin = nullptr;
			PCGDataType upstreamTypes = PCGDataType::None;
			PCGDataType downstreamTypes = PCGDataType::None;
			if (!SortPinsAndRetrieveTypes(this, otherPin, upstreamPin, downstreamPin, upstreamTypes, downstreamTypes))
			{
				return false;
			}
			assert(upstreamPin && downstreamPin);

			// Types missing
			if (upstreamTypes == PCGDataType::None || downstreamTypes == PCGDataType::None)
			{
				return false;
			}

			// Allow if there is overlap
			return !!(upstreamTypes & downstreamTypes);
		}

		bool PCGPin::AllowsMultipleConnections() const
		{
			return IsOutputPin() || m_Properties.AllowsMultipleConnections();
		}

		const Name& PCGPin::GetLabel() const
		{
			return m_Properties.label;
		}

		PCGDataType PCGPin::GetCurrentTypes() const
		{
			// TODO: Dynamic types
			return m_Properties.allowedTypes;
		}

		const String& PCGPin::GetToolTip() const
		{
			return m_Properties.tooltip;
		}

		void PCGPin::SetTooltip(const String& toolTip)
		{
			m_Properties.tooltip = toolTip;
		}
	}
}
