#pragma once

namespace cgb
{
	class standard_descriptor_cache : public descriptor_cache_interface
	{
	public:
		const descriptor_set_layout& get_or_alloc_layout(descriptor_set_layout aPreparedLayout) override;
		const descriptor_set* get_descriptor_set_from_cache(const descriptor_set& aPreparedSet) override;
		std::vector<const descriptor_set*> alloc_descriptor_sets(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts, std::vector<descriptor_set> aPreparedSets) override;
		void clear_all_descriptor_sets() override;
		void clear_all_layouts() override;

	private:
		std::unordered_set<descriptor_set_layout> mLayouts;
		std::unordered_set<descriptor_set> mSets;
	};
}
