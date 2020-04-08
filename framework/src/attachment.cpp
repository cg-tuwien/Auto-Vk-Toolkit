#include <cg_base.hpp>

namespace cgb
{
	namespace att
	{
		usage_desc& usage_desc::operator+(usage_desc& resolveAndMore)
		{
			assert(resolveAndMore.mDescriptions.size() >= 1);
			auto& mustBeResolve = resolveAndMore.mDescriptions.front();
			if (dynamic_cast<resolve*>(&resolveAndMore) != nullptr || std::get<bool>(mustBeResolve) != true) {
				throw cgb::runtime_error("A 'resolve' element must follow after a '+'");
			}
			auto& mustBeColor = mDescriptions.back();
			if (std::get<usage_type>(mustBeColor) != usage_type::color) {
				throw cgb::runtime_error("A 'resolve' operation can only be applied to 'color' attachments.");
			}
			std::get<bool>(mustBeColor) = std::get<bool>(mustBeResolve);

			// Add the rest:
			mDescriptions.insert(mDescriptions.end(), resolveAndMore.mDescriptions.begin() + 1, resolveAndMore.mDescriptions.end());
			return *this;
		}
		
	}
	
	attachment attachment::declare(std::tuple<image_format, att::sample_count> aFormatAndSamples, att::on_load aLoadOp, att::usage_desc aUsageInSubpasses, att::on_store aStoreOp)
	{
		return attachment{
			std::get<image_format>(aFormatAndSamples),
			std::get<att::sample_count>(aFormatAndSamples).mNumSamples,
			aLoadOp, aStoreOp,
			{},      {},
			std::move(aUsageInSubpasses),
			glm::vec4{ 0.0, 0.0, 0.0, 0.0 },
			1.0f, 0u
		};
	}
	
	attachment attachment::declare(image_format aFormat, att::on_load aLoadOp, att::usage_desc aUsageInSubpasses, att::on_store aStoreOp)
	{
		return declare({aFormat, att::sample_count{1}}, aLoadOp, std::move(aUsageInSubpasses), aStoreOp);
	}
	
	attachment attachment::declare_for(const image_view_t& aImageView, att::on_load aLoadOp, att::usage_desc aUsageInSubpasses, att::on_store aStoreOp)
	{
		auto& imageInfo = aImageView.get_image().config();
		auto format = image_format{ imageInfo.format };
		std::optional<image_usage> imageUsage = aImageView.get_image().usage_config();
		return declare({format, att::sample_count{to_cgb_sample_count(imageInfo.samples)}}, aLoadOp, std::move(aUsageInSubpasses), aStoreOp);
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
}