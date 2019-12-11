#pragma once

namespace cgb
{
	template <typename V, typename F>
	bool has_flag(V value, F flag)
	{
		return (value & flag) == flag;
	}
	
	/** Intended to be used to sync access to context() calls.
	 *	I.e. if you are using a parallel executor, wrap all calls to 
	 *	the context in the SYNC_CALL macro or use the SYNC_THIS_SCOPE macro!
	 */
	extern std::mutex gContextSyncMutex;

#define SYNC_THIS_SCOPE std::scoped_lock<std::mutex> guard(cgb::gContextSyncMutex);
#define SYNC_CALL(x) do { SYNC_THIS_SCOPE x; } while(false)


	/** Calls the context's @ref check_error method, passing the current file and line to it. */
#define CHECK_ERROR() context().check_error(__FILE__, __LINE__);

	/** A struct holding a context-specific function
	*	Create such a struct and use it in conjunction with the macros SET_VULKAN_FUNCTION and 
	*	SET_OPENGL46_FUNCTION to specify context-specific code.
	*
	*	Example: Create a context-specific function to select a format depending on the context used:
	*
	*	 auto selectImageFormat = cgb::context_specific_function<cgb::image_format()>{}
	*								.VK_FUNC([]() { return cgb::image_format{ vk::Format::eR8G8B8Unorm }; })
	*								.GL_FUNC([]() { return cgb::image_format{ GL_RGB }; });
	*/
	template<typename T>
	struct context_specific_function
	{
		std::function<T> mFunction;

		context_specific_function() {}

		template <typename F, typename std::enable_if<std::is_assignable_v<std::function<T>, F>, int>::type = 0>
		context_specific_function(F func)
		{
			set_function(std::move(func));
		}

		template <typename F, typename std::enable_if<std::is_assignable_v<std::function<T>, F>, int>::type = 0>
		auto& operator=(F func)
		{
			set_function(std::move(func));
			return *this;
		}

		/** Sets the function. However, this is not intended to be called directly!
		*	Use the macros VK_FUNC and GL_FUNC instead!
		*	Use those macros exactly in the same way as you would call this function;
		*	i.e. instead of .set_function(...) just write .VK_FUNC(...) for the Vulkan context.
		*/
		auto& set_function(std::function<T> func)
		{
			mFunction = std::move(func);
			return *this;
		}

		/** Function which is used by the macros SET_VULKAN_FUNCTION and SET_OPENGL46_FUNCTION.
		*	This is not intended to be called directly, so just use the macros!
		*/
		auto& do_nothing()
		{
			return *this;
		}
	};

#if defined(USE_OPENGL_CONTEXT)
#define GL_ONLY(x) x
#define VK_ONLY(x) 
#define GL_FUNC(x) set_function(x)
#define VK_FUNC(x) do_nothing()
#elif defined(USE_VULKAN_CONTEXT)
#define GL_ONLY(x)
#define VK_ONLY(x) x
#define GL_FUNC(x) do_nothing()
#define VK_FUNC(x) set_function(x)
#endif

	///** Gets a reference to the data stored in a variant, regardless of how it is stored/referenced 
	//*/
	//template <typename T>
	//T& get(T& v)	
	//{
	//	return v;
	//}

	///** Gets a reference to the data stored in a variant, regardless of how it is stored/referenced 
	//*/
	//template <typename T>
	//const T& get(const T& v)	
	//{
	//	return v;
	//}

	///** Gets a reference to the data stored in a variant, regardless of how it is stored/referenced 
	//*/
	//template <typename T>
	//T& get(T* v)	
	//{
	//	return *v;
	//}

	///** Gets a reference to the data stored in a variant, regardless of how it is stored/referenced 
	//*/
	//template <typename T>
	//const T& get(const T* v)	
	//{
	//	return *v;
	//}

	///** Gets a reference to the data stored in a variant, regardless of how it is stored/referenced there,
	//*	be it stored directly or referenced via a smart pointer.
	//*/
	//template <typename T, typename V>
	//T& get(V& v)	
	//{
	//	if (std::holds_alternative<T>(v)) {
	//		return std::get<T>(v);
	//	}
	//	if (std::holds_alternative<std::unique_ptr<T>>(v)) {
	//		return *std::get<std::unique_ptr<T>>(v);
	//	}
	//	if (std::holds_alternative<std::shared_ptr<T>>(v)) {
	//		return *std::get<std::shared_ptr<T>>(v);
	//	}
	//	throw std::bad_variant_access();
	//}

	///** Gets a reference to the data stored in a variant, regardless of how it is stored/referenced there,
	//*	be it stored directly or referenced via a smart pointer.
	//*/
	//template <typename T, typename V>
	//const T& get(const V& v)	
	//{
	//	if (std::holds_alternative<T>(v)) {
	//		return std::get<T>(v);
	//	}
	//	if (std::holds_alternative<std::unique_ptr<T>>(v)) {
	//		return *std::get<std::unique_ptr<T>>(v);
	//	}
	//	if (std::holds_alternative<std::shared_ptr<T>>(v)) {
	//		return *std::get<std::shared_ptr<T>>(v);
	//	}
	//	throw std::bad_variant_access();
	//}

	///** Gets a reference to the data stored in a variant, regardless of how it is stored/referenced there,
	//*	be it stored directly or referenced via a smart pointer.
	//*/
	//template <typename T>
	//T& get(std::variant<T, std::unique_ptr<T>, std::shared_ptr<T>>& v)	
	//{
	//	if (std::holds_alternative<T>(v)) {
	//		return std::get<T>(v);
	//	}
	//	if (std::holds_alternative<std::unique_ptr<T>>(v)) {
	//		return *std::get<std::unique_ptr<T>>(v);
	//	}
	//	if (std::holds_alternative<std::shared_ptr<T>>(v)) {
	//		return *std::get<std::shared_ptr<T>>(v);
	//	}
	//	throw std::bad_variant_access();
	//}

	///** Gets a reference to the data stored in a variant, regardless of how it is stored/referenced there,
	//*	be it stored directly or referenced via a smart pointer.
	//*/
	//template <typename T>
	//const T& get(const std::variant<T, std::unique_ptr<T>, std::shared_ptr<T>>& v)	
	//{
	//	if (std::holds_alternative<T>(v)) {
	//		return std::get<T>(v);
	//	}
	//	if (std::holds_alternative<std::unique_ptr<T>>(v)) {
	//		return *std::get<std::unique_ptr<T>>(v);
	//	}
	//	if (std::holds_alternative<std::shared_ptr<T>>(v)) {
	//		return *std::get<std::shared_ptr<T>>(v);
	//	}
	//	throw std::bad_variant_access();
	//}

	///** Transform an element into a unique_ptr by moving from it.
	// *	The parameter must be an r-value reference and will not be usable 
	// *	after this function call anymore because it has been moved from.
	// *
	// *	Example:
	// *    `buffer b = cgb::make_unique(cgb::create(cgb::vertex_buffer{1,2}, ...));`
	// */
	//template <typename T>
	//std::unique_ptr<T> make_unique(T&& t)
	//{
	//	return std::make_unique<T>(std::move(t));
	//}

	///** Transform an element into a shared_ptr by moving from it.
	//*	The parameter must be an r-value reference and will not be usable 
	//*	after this function call anymore because it has been moved from.
	//*
	//*	Example:
	//*    `buffer b = cgb::make_shared(cgb::create(cgb::vertex_buffer{1,2}, ...));`
	//*/
	//template <typename T>
	//std::shared_ptr<T> make_shared(T&& t)
	//{
	//	return std::make_shared<T>(std::move(t));
	//}

	// SFINAE test for detecting if a type has a `.size()` member and iterators
	template <typename T>
	class has_size_and_iterators
	{
	private:
		typedef char NoType[1];
		typedef char YesType[2];

		template<typename C> static auto Test(void*)
			-> std::tuple<YesType, decltype(std::declval<C const>().size()), decltype(std::declval<C const>().begin()), decltype(std::declval<C const>().end())>;

		  template<typename> static NoType& Test(...);

		public:
			static bool const value = sizeof(Test<T>(0)) != sizeof(NoType);
	};


	template<typename T> 
	typename std::enable_if<has_size_and_iterators<T>::value, uint32_t>::type how_many_elements(const T& t) {
		return static_cast<uint32_t>(t.size());
	}

	template<typename T> 
	typename std::enable_if<!has_size_and_iterators<T>::value, uint32_t>::type how_many_elements(const T& t) {
		return 1u;
	}

	template<typename T> 
	typename std::enable_if<has_size_and_iterators<T>::value, const typename T::value_type&>::type first_or_only_element(const T& t) {
		return t[0];
	}

	template<typename T> 
	typename std::enable_if<!has_size_and_iterators<T>::value, const T&>::type first_or_only_element(const T& t) {
		return t;
	}



	template <typename T>
	class owning_resource : public std::variant<std::monostate, T, std::shared_ptr<T>>
	{
	public:
		using value_type = T;

		owning_resource() : std::variant<std::monostate, T, std::shared_ptr<T>>() {}
		owning_resource(T&& r) : std::variant<std::monostate, T, std::shared_ptr<T>>(std::move(r)) {}
		owning_resource(const T&) = delete;
		owning_resource(owning_resource<T>&&) = default;
		owning_resource(const owning_resource<T>& other)
		{
			if (other.has_value()) {
				if (!other.is_shared_ownership_enabled()) {
					//
					//	Why are you getting this exception?
					//	The most obvious reason would be that you have intentionally
					//	tried to copy a resource which might not be allowed unless
					//	"shared ownership" has been enabled.
					//	If you intended to enable shared ownership, call: `enable_shared_ownership()`
					//	which will move the resource internally into a shared pointer.
					//
					//	Attention:
					//	A common source of unintentionally causing this exception is
					//	the usage of a resource in an `std::initializer_list`. The problem
					//	with that is that it looks like it does not support move-only types.
					//	An alternative to initializer lists would be to use `cgb::make_vector`.
					//
					throw std::logic_error("Can only copy construct owning_resources which have shared ownership enabled.");
				}
				*this_as_variant() = std::get<std::shared_ptr<T>>(other);
			}
			else {
				assert(!has_value());
			}
		}
		owning_resource<T>& operator=(T&& r) { *this_as_variant() = std::move(r); return *this; }
		owning_resource<T>& operator=(const T&) = delete;
		owning_resource<T>& operator=(owning_resource<T>&& r) = default;
		owning_resource<T>& operator=(const owning_resource<T>& other)
		{
			if (other.has_value()) {
				if (!other.is_shared_ownership_enabled()) {
					//
					//	Why are you getting this exception?
					//	The most obvious reason would be that you have intentionally
					//	tried to copy a resource which might not be allowed unless
					//	"shared ownership" has been enabled.
					//	If you intended to enable shared ownership, call: `enable_shared_ownership()`
					//	which will move the resource internally into a shared pointer.
					//
					//	Attention:
					//	A common source of unintentionally causing this exception is
					//	the usage of a resource in an `std::initializer_list`. The problem
					//	with that is that it looks like it does not support move-only types.
					//	An alternative to initializer lists would be to use `cgb::make_vector`.
					//
					throw std::logic_error("Can only copy assign owning_resources which have shared ownership enabled.");
				}
				*this_as_variant() = std::get<std::shared_ptr<T>>(other);
			}
			else {
				assert(!has_value());
			}
			return *this;
		}

		bool is_shared_ownership_enabled() const { return std::holds_alternative<std::shared_ptr<T>>(*this); }
		bool has_value() const { return !std::holds_alternative<std::monostate>(*this); }
		bool holds_item_directly() const { return std::holds_alternative<T>(*this); }
		const std::variant<std::monostate, T, std::shared_ptr<T>>* this_as_variant() const { return static_cast<const std::variant<std::monostate, T, std::shared_ptr<T>>*>(this); }
		std::variant<std::monostate, T, std::shared_ptr<T>>* this_as_variant() { return static_cast<std::variant<std::monostate, T, std::shared_ptr<T>>*>(this); }

		void enable_shared_ownership()
		{
			if (is_shared_ownership_enabled()) {
				return; // Already established
			}
			if (std::holds_alternative<std::monostate>(*this)) {
				throw std::logic_error("This owning_resource is uninitialized, i.e. std::monostate.");
			}
			*this_as_variant() = std::make_shared<T>(std::move(std::get<T>(*this)));
		}

		operator const T&() const 
		{ 
			if (is_shared_ownership_enabled()) { return *std::get<std::shared_ptr<T>>(*this_as_variant()); }
			if (holds_item_directly()) { return std::get<T>(*this_as_variant()); }
			throw std::logic_error("This owning_resource is uninitialized, i.e. std::monostate.");
		}

		operator T&() 
		{ 
			if (is_shared_ownership_enabled()) { return *std::get<std::shared_ptr<T>>(*this_as_variant()); }
			if (holds_item_directly()) { return std::get<T>(*this_as_variant()); }
			throw std::logic_error("This owning_resource is uninitialized, i.e. std::monostate.");
		}
		
		const T& operator*() const
		{
			return this->operator const T&();
		}
		
		T& operator*()
		{
			return this->operator T&();
		}
		
		const T* operator->() const
		{
			return &this->operator const T&();
		}
		
		T* operator->()
		{
			return &this->operator T&();
		}
	};



	template<typename T>
	class unique_function : protected std::function<T>
	{
	    template<typename Fn, typename En = void>
	    struct wrapper;

	    // specialization for CopyConstructible Fn
	    template<typename Fn>
	    struct wrapper<Fn, std::enable_if_t< std::is_copy_constructible<Fn>::value >>
	    {
	        Fn fn;

	        template<typename... Args>
	        auto operator()(Args&&... args) { return fn(std::forward<Args>(args)...); }
	    };

	    // specialization for MoveConstructible-only Fn
	    template<typename Fn>
	    struct wrapper<Fn, std::enable_if_t< !std::is_copy_constructible<Fn>::value
											 && std::is_move_constructible<Fn>::value >>
	    {
	        Fn fn;

	        wrapper(Fn fn) : fn(std::move(fn)) { }

	        wrapper(wrapper&&) = default;
	        wrapper& operator=(wrapper&&) = default;

	        // these two functions are instantiated by std::function and are never called
	        wrapper(const wrapper& rhs) : fn(const_cast<Fn&&>(rhs.fn)) { throw std::logic_error("never called"); } // hack to initialize fn for non-DefaultContructible types
	        wrapper& operator=(const wrapper&) { throw std::logic_error("never called"); }

			~wrapper() = default;

	        template<typename... Args>
	        auto operator()(Args&&... args) { return fn(std::forward<Args>(args)...); }
	    };

	    using base = std::function<T>;

	public:
		unique_function() = default;
		unique_function(std::nullptr_t) : base(nullptr) {}
		unique_function(const unique_function&) = default;
		unique_function(unique_function&&) = default;

		template<class Fn> 
		unique_function(Fn f) : base( wrapper<Fn>{ std::move(f) }) {}

		~unique_function() = default;

		unique_function& operator=(const unique_function& other) = default;
		unique_function& operator=(unique_function&& other) = default;
		unique_function& operator=(std::nullptr_t)
		{
			base::operator=(nullptr); return *this;
		}

		template<typename Fn>
	    unique_function& operator=(Fn&& f)
	    { base::operator=(wrapper<Fn>{ std::forward<Fn>(f) }); return *this; }

		template<typename Fn>
	    unique_function& operator=(std::reference_wrapper<Fn> f)
	    { base::operator=(wrapper<Fn>{ std::move(f) }); return *this; }

		template<class Fn> 
		Fn* target()
		{ return &base::template target<wrapper<Fn>>()->fn; }

		template<class Fn> 
		Fn* target() const
		{ return &base::template target<wrapper<Fn>>()->fn; }

		template<class Fn> 
		const std::type_info& target_type() const
		{ return typeid(*target<Fn>()); }

		using base::swap;
	    using base::operator();
		using base::operator bool;
	};

		
	template< class R, class... Args >
	static void swap( unique_function<R(Args...)> &lhs, unique_function<R(Args...)> &rhs )
	{
		lhs.swap(rhs);
	}

	template< class R, class... ArgTypes >
	static bool operator==( const unique_function<R(ArgTypes...)>& f, std::nullptr_t )
	{
		return !f;
	}

	template< class R, class... ArgTypes >
	static bool operator==( std::nullptr_t, const unique_function<R(ArgTypes...)>& f )
	{
		return !f;
	}

	template< class R, class... ArgTypes >
	static bool operator!=( const unique_function<R(ArgTypes...)>& f, std::nullptr_t )
	{
		return static_cast<bool>(f);
	}

	template< class R, class... ArgTypes >
	static bool operator!=( std::nullptr_t, const unique_function<R(ArgTypes...)>& f )
	{
		return static_cast<bool>(f);
	}

	/*	Combines multiple hash values.
	 *  Inspiration and implementation largely taken from: https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x/54728293#54728293
	 *  TODO: Should a larger magic constant be used since we're only supporting x64 and hence, size_t will always be 64bit?!
	 */
	template <typename T, typename... Rest>
	void hash_combine(std::size_t& seed, const T& v, const Rest&... rest)
	{
		seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		(hash_combine(seed, rest), ...);
	}

	/**	Returns true if `_Element` is contained within `_Vector`, also provides 
	 *	the option to return the position where the element has been found.
	 *	@param	_Vector			Where to look for `_Element`
	 *	@param	_Element		The element to find
	 *	@param	_OutPosition	(Optional) If the address of an iterator is set, it will be assigned the position of the element
	 */
	template <typename T>
	bool contains_element(const std::vector<T>& _Vector, const T& _Element, typename std::vector<T>::const_iterator* _OutPosition = nullptr)
	{
		auto it = std::find(std::begin(_Vector), std::end(_Vector), _Element);
		if (nullptr != _OutPosition) {
			*_OutPosition = it;
		}
		return _Vector.end() != it;
	}

	/** Returns the index of the first occurence of `_Element` within `_Vector`
	 *	If the element is not contained, `_Vector.size()` will be returned.
	 */
	template <typename T>
	size_t index_of(const std::vector<T>& _Vector, const T& _Element)
	{
		auto it = std::find(std::begin(_Vector), std::end(_Vector), _Element);
		return std::distance(std::begin(_Vector), it);
	}

	/** Inserts a copy of `_Element` into `_Vector` if `_Element` is not already contained in `_Vector` 
	 *	@param	_Vector				The collection where `_Element` shall be inserted
	 *	@param	_Element			The element to be inserted into `_Vector`, if it is not already contained. `_Element` must be copy-constructible.
	 *	@param	_PositionOfElement	(Optional) If the address of an iterator is set, it will be assigned the position of the 
	 *								newly inserted element into `_Vector`.
	 *	@return	True if the element was inserted into the vector, meaning it was not contained before. False otherwise.
	 */
	template <typename T>
	bool add_to_vector_if_not_already_contained(std::vector<T>& _Vector, const T& _Element, typename std::vector<T>::const_iterator* _PositionOfElement = nullptr)
	{
		if (!contains_element(_Vector, _Element, _PositionOfElement)) {
			_Vector.push_back(_Element);
			if (nullptr != _PositionOfElement) {
				*_PositionOfElement = std::prev(_Vector.end());
			}
			return true;
		}
		return false;
	}

}
