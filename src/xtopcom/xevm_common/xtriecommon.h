// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/common.h"
#include "xcommon/rlp.h"
#include "xutility/xhash.h"

namespace top {
namespace evm_common {

bool sha3(bytesConstRef _input, bytesRef o_output) noexcept;

inline h256 sha3(bytesConstRef _input) noexcept
{
    h256 ret;
    sha3(_input, ret.ref());
    return ret;
}

inline h256 sha3(xbytes_t const & _input) noexcept {
    return  sha3(bytesConstRef(&_input));
}

inline h256 sha3(h256 const& _input) noexcept
{
    h256 ret;
	const uint256_t top_hash = utl::xkeccak256_t::digest((const unsigned char *)_input.data(), 32);
	bytesConstRef((const unsigned char*)top_hash.data(), 32).copyTo(ret.ref());

    return ret;
}


extern const h256 EmptyTrie;

inline xbyte_t nibble(bytesConstRef _data, unsigned _i)
{
	return (_i & 1) ? (_data[_i / 2] & 15) : (_data[_i / 2] >> 4);
}

/// Interprets @a _first and @a _second as vectors of nibbles and returns the length of the longest common
/// prefix of _first[_beginFirst..._endFirst] and _second[_beginSecond..._endSecond].
inline unsigned sharedNibbles(bytesConstRef _first, unsigned _beginFirst, unsigned _endFirst, bytesConstRef _second, unsigned _beginSecond, unsigned _endSecond)
{
	unsigned ret = 0;
	while (_beginFirst < _endFirst && _beginSecond < _endSecond && nibble(_first, _beginFirst) == nibble(_second, _beginSecond))
	{
		++_beginFirst;
		++_beginSecond;
		++ret;
	}
	return ret;
}

/**
 * Nibble-based view on a bytesConstRef.
 */
struct NibbleSlice
{
	bytesConstRef data;
	unsigned offset;

	NibbleSlice(bytesConstRef _data = bytesConstRef(), unsigned _offset = 0): data(_data), offset(_offset) {}
	xbyte_t operator[](unsigned _index) const { return nibble(data, offset + _index); }
	unsigned size() const { return data.size() * 2 - offset; }
	bool empty() const { return !size(); }
	NibbleSlice mid(unsigned _index) const { return NibbleSlice(data, offset + _index); }
	void clear() { data.reset(); offset = 0; }

	/// @returns true iff _k is a prefix of this.
	bool contains(NibbleSlice _k) const { return shared(_k) == _k.size(); }
	/// @returns the number of shared nibbles at the beginning of this and _k.
	unsigned shared(NibbleSlice _k) const { return sharedNibbles(data, offset, offset + size(), _k.data, _k.offset, _k.offset + _k.size()); }
	/**
	 * @brief Determine if we, a full key, are situated prior to a particular key-prefix.
	 * @param _k The prefix.
	 * @return true if we are strictly prior to the prefix.
	 */
	bool isEarlierThan(NibbleSlice _k) const
	{
		unsigned i = 0;
		for (; i < _k.size() && i < size(); ++i)
			if (operator[](i) < _k[i])		// Byte is lower - we're earlier..
				return true;
			else if (operator[](i) > _k[i])	// Byte is higher - we're not earlier.
				return false;
		if (i >= _k.size())					// Ran past the end of the prefix - we're == for the entire prefix - we're not earlier.
			return false;
		return true;						// Ran out before the prefix had finished - we're earlier.
	}
	bool operator==(NibbleSlice _k) const { return _k.size() == size() && shared(_k) == _k.size(); }
	bool operator!=(NibbleSlice _s) const { return !operator==(_s); }
};

inline std::ostream& operator<<(std::ostream& _out, NibbleSlice const& _m)
{
	for (unsigned i = 0; i < _m.size(); ++i)
		_out << std::hex << (int)_m[i] << std::dec;
	return _out;
}

/*
inline bool isLeaf(RLP const& _twoItem)
{
	assert(_twoItem.isList() && _twoItem.itemCount() == 2);
	auto pl = _twoItem[0].payload();
	return (pl[0] & 0x20) != 0;
}
*/
/*
inline NibbleSlice keyOf(bytesConstRef _hpe)
{
	if (!_hpe.size())
		return NibbleSlice(_hpe, 0);
	if (_hpe[0] & 0x10)
		return NibbleSlice(_hpe, 1);
	else
		return NibbleSlice(_hpe, 2);
}*/
/*
inline NibbleSlice keyOf(RLP const& _twoItem)
{
	return keyOf(_twoItem[0].payload());
}
*/
xbyte_t uniqueInUse(RLP const& _orig, xbyte_t except);
std::string hexPrefixEncode(xbytes_t const & _hexVector, bool _leaf = false, int _begin = 0, int _end = -1);
std::string hexPrefixEncode(bytesConstRef _data, bool _leaf, int _beginNibble, int _endNibble, unsigned _offset);
std::string hexPrefixEncode(bytesConstRef _d1, unsigned _o1, bytesConstRef _d2, unsigned _o2, bool _leaf);

inline std::string hexPrefixEncode(NibbleSlice _s, bool _leaf, int _begin = 0, int _end = -1)
{
	return hexPrefixEncode(_s.data, _leaf, _begin, _end, _s.offset);
}

inline std::string hexPrefixEncode(NibbleSlice _s1, NibbleSlice _s2, bool _leaf)
{
	return hexPrefixEncode(_s1.data, _s1.offset, _s2.data, _s2.offset, _leaf);
}

}
}