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
				throw std::runtime_error("A 'resolve' element must follow after a '+'");
			}
			auto& mustBeColor = mDescriptions.back();
			if (std::get<usage_type>(mustBeColor) != usage_type::color) {
				throw std::runtime_error("A 'resolve' operation can only be applied to 'color' attachments.");
			}
			std::get<bool>(mustBeColor) = std::get<bool>(mustBeResolve);

			// Add the rest:
			mDescriptions.insert(mDescriptions.end(), resolveAndMore.mDescriptions.begin() + 1, resolveAndMore.mDescriptions.end());
			return *this;
		}
		
	}
	
	attachment attachment::define(std::tuple<image_format, cfg::sample_count> aFormatAndSamples, cfg::attachment_load_operation aLoadOp, att::usage_desc aUsageInSubpasses, cfg::attachment_store_operation aStoreOp)
	{
		return attachment{
			std::get<image_format>(aFormatAndSamples),
			std::get<cfg::sample_count>(aFormatAndSamples).mNumSamples,
			aLoadOp, aStoreOp,
			{},      {},
			std::move(aUsageInSubpasses)
		};
	}
	
	attachment attachment::define(image_format aFormat, cfg::attachment_load_operation aLoadOp, att::usage_desc aUsageInSubpasses, cfg::attachment_store_operation aStoreOp)
	{
		return define({aFormat, cfg::sample_count{1}}, aLoadOp, std::move(aUsageInSubpasses), aStoreOp);
	}
	
	attachment attachment::define_for(const image_view_t& aImageView, cfg::attachment_load_operation aLoadOp, att::usage_desc aUsageInSubpasses, cfg::attachment_store_operation aStoreOp)
	{
		auto& imageInfo = aImageView.get_image().config();
		auto format = image_format{ imageInfo.format };
		std::optional<image_usage> imageUsage = aImageView.get_image().usage_config();
		return define({format, cfg::sample_count{to_cgb_sample_count(imageInfo.samples)}}, aLoadOp, std::move(aUsageInSubpasses), aStoreOp);
	}

	attachment& attachment::set_load_operation(cfg::attachment_load_operation aLoadOp)
	{
		mLoadOperation = aLoadOp;
		return *this;
	}
	
	attachment& attachment::set_store_operation(cfg::attachment_store_operation aStoreOp)
	{
		mStoreOperation = aStoreOp;
		return *this;
	}

	attachment& attachment::load_contents()
	{
		return set_load_operation(cfg::attachment_load_operation::load);
	}
	
	attachment& attachment::clear_contents()
	{
		return set_load_operation(cfg::attachment_load_operation::clear);
	}
	
	attachment& attachment::store_contents()
	{
		return set_store_operation(cfg::attachment_store_operation::store);
	}

	attachment& attachment::set_stencil_load_operation(cfg::attachment_load_operation aLoadOp)
	{
		mStencilLoadOperation = aLoadOp;
		return *this;
	}
	
	attachment& attachment::set_stencil_store_operation(cfg::attachment_store_operation aStoreOp)
	{
		mStencilStoreOperation = aStoreOp;
		return *this;
	}
	
	attachment& attachment::load_stencil_contents()
	{
		return set_stencil_load_operation(cfg::attachment_load_operation::load);
	}
	
	attachment& attachment::clear_stencil_contents()
	{
		return set_stencil_load_operation(cfg::attachment_load_operation::clear);
	}
	
	attachment& attachment::store_stencil_contents()
	{
		return set_stencil_store_operation(cfg::attachment_store_operation::store);
	}
}