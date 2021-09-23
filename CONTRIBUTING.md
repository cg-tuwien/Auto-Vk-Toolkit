# Contribution Guidelines

If you are contributing to cg_base, please follow these guidelines!

## C++

Always use the newest C++ features available! Never go for an old approach, if there is a new approach available.
Strive to write as modern C++ as possible!

Some examples:
* Never use plain C-style arrays, but use `std::array` instead!
* Never use `new` but (in most cases/where appropriate) use `std::unique_ptr` instead
* Do not use a `bool` return value plus an 'out'-parameter to return a value, but use `std::optional<my_out_type>` instead

### Vulkan HPP

Especially for Vulkan, try to use the C-interface as little as possible and fall back to it only if there are good reasons to do so. Always use the C++ equivalents if available! E.g. don't use `VkInstance` but use `vk::Instance`!

## Naming Conventions

```
namespace namespace_name_lower_case_separated_by_underscores
{ // Braces for namespaces, types, methods start in the next line

    /** @brief Comment every type in a meaningful manner
     *	
     *	The comments should be a help to the users of the class/method/function/...
     */
    class type_name_lower_case_separated_by_underscores
    { 
    public:
        /** Comment at least all public members of a class */
        void method_name_lower_case_separated_by_underscores() const
        {
            int localVariablesInCamelCase = 0; // Use explanatory comments whenever neccessary

            while (true) { // Braces for loops, ifs, switches, scoped blocks start in the same line
                single_statements(); // Prefer to wrap them in braces regardless!
            }
        }

        /** Getter of member variable mValue is named value */
        complex_type value() const // Return by value unless there are good reasons not to!
        {
            return mValue; // no conflict with value here
        }

        /** Setter of member variable mValue is called set_value 
         *
         *  Arguments should be prefixed with 'a' and are always named in camel case.
         *  With prefixes, all names are distinct: mValue, aValue, value(), set_value()
         */
        void set_value(const complex_type& aValue)
        {
            mValue = aValue; // no conflict with mValue or value here
        }

    protected:
        // The more public it is, the more important it is. Therefore,
        // order like follows: public on top, then protected, then private

        template <typename Template, typename Parameters>
        void templated_method()
        {
            // Template type parameters are named in camel case,
            // starting with a capital letter.
        }

    private:
        // Prefix members with 'm', variable name in camel case
        complex_type mValue;
    };
}

// Thanks to StackOverflow user GManNickG for providing guidelines very similar to these

// New line at the end of the file:

```

## Further Guidelines

### [[nodiscard]]

Apply ``[[nodiscard]`` to functions/methods if discarding a return value indicates a memory or resource leak, or a common programmer error.

An example of a common programmer error would be not using the return value of [`std::vector::empty`](https://en.cppreference.com/w/cpp/container/vector/empty). The reasoning is: Maybe the programmer wanted to empty the vector, but that is not what [`std::vector::empty`](https://en.cppreference.com/w/cpp/container/vector/empty) does.

_Please note:_ Please name functions/methods so that such confusions like in that example can be avoided in the first place! Use `is_empty` instead of `empty` to express the functionality more clearly!
