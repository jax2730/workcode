#pragma once

#include "EchoComponentPrerequisites.h"

#include "PCG/EchoPCGCommon.h"

namespace Echo
{
	namespace PCGV2
	{
		enum class EPCGElementLogMode : uint8
		{
			/** Output to log. */
			LogOnly = 0,
			/** Display errors/warnings on graph as well as writing to log. */
			GraphAndLog,

			NumLogModes,

			// Used below in log macros to silence PVS by making the log mode comparison 'look' non-trivial by adding a trivial mask op (an identical
			// mechanism as the one employed in the macro UE_ASYNC_PACKAGE_LOG in AsyncLoading2.cpp).
			// Warning V501: There are identical sub-expressions 'EPCGElementLogMode::GraphAndLog' to the left and to the right of the '==' operator.
			// The warning disable comment can can't be used in a macro: //-V501 
			LogModeMask = 0xff
		};

		enum class PCGElementExecutionLoopMode : uint8
		{
			/** Not a trivial input -> output mapping, with respect to caching. */
			NotALoop,
			/** Loops on (singular) required pin. */
			SinglePrimaryPin,
			/** Loops on matching indices on required pin(s). */
			MatchingPrimaryPins,
			/** Cartesian loop on required pins. */
			// CartesianPins // TODO
		};

		enum class PCGCachingStatus : uint8
		{
			NotCacheable,
			NotInCache,
			Cached
		};

		class _EchoComponentExport PCGElement
		{
		public:
			virtual ~PCGElement() = default;

			virtual PCGContext* Initialize(const PCGDataCollection& inputData, const PCGNode* node);

			bool Execute(PCGContext* context);

		protected:

			virtual PCGContext* CreateContext();

			virtual bool PrepareDataInternal(PCGContext* inContext);

			virtual bool ExecuteInternal(PCGContext* context) = 0;

			virtual PCGElementExecutionLoopMode ExecutionLoopMode(const PCGV2::PCGSettings* Settings) const { return PCGElementExecutionLoopMode::NotALoop; }
		};

		class _EchoComponentExport PCGEmptyElement : public PCGElement
		{
		public:
			static PCGElement* GetInstance();

		protected:
			PCGEmptyElement() = default;

			~PCGEmptyElement() override = default;

			bool ExecuteInternal(PCGContext* inContext) override;
		};
	}
}
