#include "graphics/pipeline.h"

namespace Graphics
{
	Pipeline::Pipeline(const char** resourceNames)
	    : resourceNames_(resourceNames)
	{
		while(*resourceNames != nullptr)
			++resourceNames;
		resources_.resize((i32)(resourceNames - resourceNames_));
	}

	Pipeline::~Pipeline() {}

	Core::ArrayView<const char*> Pipeline::GetResourceNames() const
	{
		return Core::ArrayView<const char*>(resourceNames_, resources_.size());
	}

	i32 Pipeline::GetResourceIdx(const char* name) const
	{
		const auto& resourceNames = GetResourceNames();
		for(i32 idx = 0; idx < resourceNames.size(); ++idx)
			if(strcmp(resourceNames[idx], name) == 0)
				return idx;
		return -1;
	}

	void Pipeline::SetResource(i32 idx, RenderGraphResource res)
	{
		if(idx >= 0 && idx < resources_.size())
			resources_[idx] = res;
	}

	RenderGraphResource Pipeline::GetResource(i32 idx) const
	{
		if(idx >= 0 && idx < resources_.size())
			return resources_[idx];
		return Graphics::RenderGraphResource();
	}

	void Pipeline::SetResource(const char* name, RenderGraphResource res) { SetResource(GetResourceIdx(name), res); }

	RenderGraphResource Pipeline::GetResource(const char* name) const { return GetResource(GetResourceIdx(name)); }


} // namespace Graphics
