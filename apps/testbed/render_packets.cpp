#pragma once

#include "render_packets.h"

#include "gpu/command_list.h"
#include "gpu/types.h"

#include <algorithm>

namespace Testbed
{
	bool operator<(const RenderPacketBase& inA, const RenderPacketBase& inB)
	{
		if(inA.type_ < inB.type_)
			return true;
		else if(inA.type_ > inB.type_)
			return false;

		switch(inA.type_)
		{
		case RenderPacketType::MESH:
		{
			const MeshRenderPacket& a = static_cast<const MeshRenderPacket&>(inA);
			const MeshRenderPacket& b = static_cast<const MeshRenderPacket&>(inB);

			if(a.db_ < b.db_)
				return true;
			else if(a.db_ > b.db_)
				return false;

			auto draw = memcmp(&a.draw_, &b.draw_, sizeof(a.draw_));
			if(draw < 0)
				return true;
			else if(draw > 0)
				return false;

			auto techDesc = memcmp(&a.techDesc_, &b.techDesc_, sizeof(a.techDesc_));
			if(techDesc < 0)
				return true;
			else if(techDesc > 0)
				return false;

			if(a.shader_ < b.shader_)
				return true;
			else if(a.shader_ > b.shader_)
				return false;

			if(a.techs_ < b.techs_)
				return true;
			else if(a.techs_ > b.techs_)
				return false;
		}
		break;

			break;
		}

		return false;
	}

	void SortPackets(Core::Vector<RenderPacketBase*>& packets) { std::sort(packets.begin(), packets.end()); }

	void MeshRenderPacket::DrawPackets(Core::ArrayView<MeshRenderPacket*> packets, Core::ArrayView<i32> passTechIndices,
	    GPU::CommandList& cmdList, const GPU::DrawState& drawState, GPU::Handle fbs, GPU::Handle viewCBHandle,
	    GPU::Handle objectSBHandle, CustomBindFn customBindFn)
	{
		// Allocate command list memory.
		auto* objects = cmdList.Alloc<Testbed::ObjectConstants>(packets.size());

		// Update all render packet uniforms.
		for(i32 idx = 0; idx < packets.size(); ++idx)
			objects[idx] = packets[idx]->object_;
		cmdList.UpdateBuffer(objectSBHandle, 0, sizeof(Testbed::ObjectConstants) * packets.size(), objects);

		i32 baseInstanceIdx = 0;
		i32 numInstances = 0;

		// Draw all packets, instanced where possible.
		const Testbed::MeshRenderPacket* nextMeshPacket = nullptr;
		const i32 objectDataSize = sizeof(Testbed::ObjectConstants);
		for(i32 idx = 0; idx < packets.size(); ++idx)
		{
			const auto* meshPacket = packets[idx];
			i32 passTechIdx = passTechIndices[idx];

			auto& tech = meshPacket->techs_->passTechniques_[passTechIdx];
			bool doDraw = true;
			if(customBindFn)
				doDraw = customBindFn(meshPacket->shader_, tech);

			if(doDraw)
				++numInstances;

			// If not instancable with the next mesh packet, on the last one, then draw last batch and start over.
			nextMeshPacket = ((idx + 1) < packets.size()) ? packets[idx + 1] : nullptr;
			if(nextMeshPacket == nullptr || !meshPacket->IsInstancableWith(*nextMeshPacket))
			{
				if(numInstances > 0)
				{
					tech.Set("ViewCBuffer", GPU::Binding::CBuffer(viewCBHandle, 0, sizeof(Testbed::ViewConstants)));
					tech.Set("inObject",
					    GPU::Binding::Buffer(
					        objectSBHandle, GPU::Format::INVALID, baseInstanceIdx, numInstances, objectDataSize));
					if(auto pbs = tech.GetBinding())
					{
						cmdList.Draw(pbs, meshPacket->db_, fbs, drawState, GPU::PrimitiveTopology::TRIANGLE_LIST,
						    meshPacket->draw_.indexOffset_, meshPacket->draw_.vertexOffset_,
						    meshPacket->draw_.noofIndices_, 0, numInstances);
					}
				}

				baseInstanceIdx = idx + 1;
				numInstances = 0;
			}
		}
	}


} // namespace Testbed
