#include <cg_base.hpp>

namespace cgb
{
	namespace att
	{
		usage_desc& usage_desc::operator+(usage_desc additionalUsages)
		{
			assert(additionalUsages.mDescriptions.size() >= 1);
			auto& additional = additionalUsages.mDescriptions.front();
			auto& existing = mDescriptions.back();

			existing.mInput	= existing.mInput || additional.mInput;
			existing.mColor = existing.mColor || additional.mColor;
			existing.mDepthStencil = existing.mDepthStencil || additional.mDepthStencil;
			existing.mPreserve = existing.mPreserve || additional.mPreserve;
			assert(existing.mInputLocation == -1 || additional.mInputLocation == -1);
			existing.mInputLocation = std::max(existing.mInputLocation, additional.mInputLocation);
			assert(existing.mColorLocation == -1 || additional.mColorLocation == -1);
			existing.mColorLocation	= std::max(existing.mColorLocation, additional.mColorLocation);
			existing.mResolve = existing.mResolve || additional.mResolve;
			assert(existing.mResolveAttachmentIndex == -1 || additional.mResolveAttachmentIndex == -1);
			existing.mResolveAttachmentIndex = std::max(existing.mResolveAttachmentIndex, additional.mResolveAttachmentIndex);
			
			// Add the rest:
			mDescriptions.insert(mDescriptions.end(), additionalUsages.mDescriptions.begin() + 1, additionalUsages.mDescriptions.end());
			return *this;
		}
		
	}
	
	attachment attachment::declare(std::tuple<image_format, int> aFormatAndSamples, att::on_load aLoadOp, att::usage_desc aUsageInSubpasses, att::on_store aStoreOp)
	{
		return attachment{
			std::get<image_format>(aFormatAndSamples),
			std::get<int>(aFormatAndSamples),
			aLoadOp, aStoreOp,
			{},      {},
			std::move(aUsageInSubpasses),
			glm::vec4{ 0.0, 0.0, 0.0, 0.0 },
			1.0f, 0u
		};
	}
	
	attachment attachment::declare(image_format aFormat, att::on_load aLoadOp, att::usage_desc aUsageInSubpasses, att::on_store aStoreOp)
	{
		return declare({aFormat, 1}, aLoadOp, std::move(aUsageInSubpasses), aStoreOp);
	}
	
	attachment attachment::declare_for(const image_view_t& aImageView, att::on_load aLoadOp, att::usage_desc aUsageInSubpasses, att::on_store aStoreOp)
	{
		const auto& imageInfo = aImageView.get_image().config();
		const auto format = image_format{ imageInfo.format };
		const std::optional<image_usage> imageUsage = aImageView.get_image().usage_config();
		auto result = declare({format, to_cgb_sample_count(imageInfo.samples)}, aLoadOp, std::move(aUsageInSubpasses), aStoreOp);
		if (imageUsage.has_value()) {
			result.set_image_usage_hint(imageUsage.value());
		}
		return result;
	}

	attachment& attachment::set_clear_color(glm::vec4 aColor)
	{
		mColorClearValue = aColor;
		return *this;
	}
	
	attachment& attachment::set_depth_clear_value(float aDepthClear)
	{
		mDepthClearValue = aDepthClear;
		return *this;
	}
	
	attachment& attachment::set_stencil_clear_value(uint32_t aStencilClear)
	{
		mStencilClearValue = aStencilClear;
		return *this;
	}
	
	attachment& attachment::set_load_operation(att::on_load aLoadOp)
	{
		mLoadOperation = aLoadOp;
		return *this;
	}
	
	attachment& attachment::set_store_operation(att::on_store aStoreOp)
	{
		mStoreOperation = aStoreOp;
		return *this;
	}

	attachment& attachment::load_contents()
	{
		return set_load_operation(att::on_load::load);
	}
	
	attachment& attachment::clear_contents()
	{
		return set_load_operation(att::on_load::clear);
	}
	
	attachment& attachment::store_contents()
	{
		return set_store_operation(att::on_store::store);
	}

	attachment& attachment::set_stencil_load_operation(att::on_load aLoadOp)
	{
		mStencilLoadOperation = aLoadOp;
		return *this;
	}
	
	attachment& attachment::set_stencil_store_operation(att::on_store aStoreOp)
	{
		mStencilStoreOperation = aStoreOp;
		return *this;
	}
	
	attachment& attachment::load_stencil_contents()
	{
		return set_stencil_load_operation(att::on_load::load);
	}
	
	attachment& attachment::clear_stencil_contents()
	{
		return set_stencil_load_operation(att::on_load::clear);
	}
	
	attachment& attachment::store_stencil_contents()
	{
		return set_stencil_store_operation(att::on_store::store);
	}

	attachment& attachment::set_image_usage_hint(cgb::image_usage aImageUsageBeforeAndAfter)
	{
		mImageUsageHintBefore = aImageUsageBeforeAndAfter;
		mImageUsageHintAfter = aImageUsageBeforeAndAfter;
		return *this;
	}

	attachment& attachment::set_image_usage_hints(cgb::image_usage aImageUsageBefore, cgb::image_usage aImageUsageAfter)
	{
		mImageUsageHintBefore = aImageUsageBefore;
		mImageUsageHintAfter = aImageUsageAfter;
		return *this;
	}
}