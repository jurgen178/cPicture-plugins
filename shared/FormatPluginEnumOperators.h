#pragma once

inline PLUGIN_TYPE operator|(const PLUGIN_TYPE t1, const PLUGIN_TYPE t2)
{
	return static_cast<PLUGIN_TYPE>(static_cast<unsigned int>(t1) | static_cast<unsigned int>(t2));
}

inline PLUGIN_TYPE operator&(const PLUGIN_TYPE t1, const PLUGIN_TYPE t2)
{
	return static_cast<PLUGIN_TYPE>(static_cast<unsigned int>(t1) & static_cast<unsigned int>(t2));
}

inline info_type operator|(const info_type t1, const info_type t2)
{
	return static_cast<info_type>(static_cast<unsigned int>(t1) | static_cast<unsigned int>(t2));
}

inline info_type operator&(const info_type t1, const info_type t2)
{
	return static_cast<info_type>(static_cast<unsigned int>(t1) & static_cast<unsigned int>(t2));
}

inline info_type operator|=(info_type& t1, const info_type t2)
{
	return t1 = t1 | t2;
}
