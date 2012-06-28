﻿//  boost/interop/conversion_iterator.hpp  --------------------------------------------------//

//  Copyright Beman Dawes 2011
//  Copyright (c) 2004 John Maddock

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//--------------------------------------------------------------------------------------//
//                                                                                      //
//        Iterator adapters that perform string type and encoding conversion            //
//                                                                                      //
//--------------------------------------------------------------------------------------//

//--------------------------------------------------------------------------------------//
//  John Maddock's boost/regex/pending/unicode_iterator.hpp introduced me to the idea   //
//  of performing conversion via iterator adapters. The code below for the UTF-8        //
//  to/from UTF-32 and UTF-16 to/from UTF-32 adapers was based on that header.          //
//--------------------------------------------------------------------------------------//

#if !defined(BOOST_INTEROP_ITERATOR_ADAPTER_HPP)
#define BOOST_INTEROP_ITERATOR_ADAPTER_HPP

#include <boost/interop/detail/config.hpp>
#include <boost/interop/string_types.hpp>
#include <boost/cstdint.hpp>
#include <boost/assert.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/static_assert.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <stdexcept>
#include <sstream>
#include <ios>
#include <limits.h> // CHAR_BIT

namespace boost
{
namespace interop
{
namespace detail
{
  // for the proof-of-concept, simply linking in codec tables is sufficient
  extern const boost::u16_t  to_utf16[];  
  extern const unsigned char to_char[];
  extern const boost::uint8_t slice_index[];
}

/*  TODO:

  *  Add wchar_t support

  *  For char, provide:
      -- Windows runtime selection of codec
  
  *  by_null should do the character_traits eof dance

  *  John's code in <u8_t> from_iterator has been modified enough that it needs a
     complete test of its own.
     See Markus Kuhn's http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt as a
     possible test file.

  *  John's "Checked constructor" adds safety. Apply it to the range case? (But we don't
     support BidirectionalIterators.) 

*/

//--------------------------------------------------------------------------------------//

//  For n encodings, it is desirable to provide 2n rather than n squared adapters.
//  This is achieved conceptually by converting to and from UTF-32 as
//  an intermediate encoding, and then composing a conversion_iterator from one
//  from_iterator and one to_iterator. For efficiency, specializations of
//  conversion_iterator can provide direct conversion without an intermedate UTF32 step.

//--------------------------------------------------------------------------------------//

//---------------------------  end-detection policy classes  ---------------------------//

//  from_iterator needs to know when the end of the sequence is reached. There are
//  several approaches to determine this; by half-open range, by size, and by
//  null-termination. End-detection policies allow the end to be efficiently determined,
//  and to do so without adding data members needed only for specific approaches.

struct by_null_policy {};
struct by_range_policy {};
struct by_size_policy {};

template <class InputIterator>
class by_null
{
public:
  typedef by_null_policy policy_type;
  bool is_end(InputIterator itr) const
  {
    return *itr ==
      static_cast<typename std::iterator_traits<InputIterator>::value_type>(0);
  }
  void advance(std::size_t sz=1) const {}
};

template <class InputIterator>
class by_range
{
  InputIterator m_end;
public:
  typedef by_range_policy policy_type;
  by_range() {}
  by_range(InputIterator itr) : m_end(itr) {}

  void end(InputIterator e) { m_end = e; }

  bool is_end(InputIterator itr) const { return itr == m_end; }
  void advance(std::size_t sz=1) const {}
};

template <class InputIterator>
class by_size
{
  mutable std::size_t m_size;
public:
  typedef by_size_policy policy_type;
  by_size() {}
  by_size(std::ptrdiff_t n) : m_size(n) {}

  void size(std::size_t sz) { m_size = sz; }

  bool is_end(InputIterator itr) const { return m_size <= 0; }
  void advance(std::size_t sz=1) const { m_size -= sz; }
};

//-------------------------  DefaultCtorEndIterator Requirements  ----------------------//
//
//  For an iterator of type T, T() constructs the end iterator. 

//-----------------------------  from_iterator  --------------------------------//
//
//  from_iterator meets the DefaultCtorEndIterator requirements.
//  iterator_traits<InputIterator>::value_type must be FromCharT

  // primary template; partial specializations *must* be provided
  template <class InputIterator, class FromCharT, template<class> class EndPolicy>  
  class from_iterator; 

  template <class InputIterator, template<class> class EndPolicy>
  class from_iterator<InputIterator, u8_t, EndPolicy>;

  template <class InputIterator, template<class> class EndPolicy>
  class from_iterator<InputIterator, u16_t, EndPolicy>;

  template <class InputIterator, template<class> class EndPolicy>
  class from_iterator<InputIterator, u32_t, EndPolicy>;

  template <class InputIterator, template<class> class EndPolicy>
  class from_iterator<InputIterator, char, EndPolicy>;

  template <class InputIterator, template<class> class EndPolicy>
  class from_iterator<InputIterator, wchar_t, EndPolicy>;

//-----------------------------  to_iterator  --------------------------------//
//
//  to_iterator meets the DefaultCtorEndIterator requirements.
//  InputIterator must meet the DefaultCtorEndIterator requirements.
//  iterator_traits<InputIterator>::value_type must be c32_t

  // primary template; partial specializations *must* be provided
  template <class InputIterator, class ToCharT>  
  class to_iterator; 

  template <class InputIterator>
  class to_iterator<InputIterator, u8_t>;

  template <class InputIterator>
  class to_iterator<InputIterator, u16_t>;

  template <class InputIterator>
  class to_iterator<InputIterator, u32_t>;
  
  template <class InputIterator>
  class to_iterator<InputIterator, char>;

  template <class InputIterator>
  class to_iterator<InputIterator, wchar_t>;

//--------------------------------  utf-8 iterators  -----------------------------------//

  template <class InputIterator, class FromCharT, template<class> class EndPolicy>  
  //  iterator_traits<InputIterator>::value_type must be char
  class from_utf8;  // value_type is c32_t

  template <class InputIterator, class ToCharT = charT>
  //  iterator_traits<InputIterator>::value_type must be c32_t
  //  ToCharT must be char
  class to_utf8;  // value_type is char

//--------------------------------  policy_iterator  -----------------------------------//

  template <class InputIterator, template<class> class EndPolicy>
  class policy_iterator;

//--------------------------------  conversion_iterator  ------------------------------------//
//
//  conversion_iterator meets the DefaultCtorEndIterator requirements.
//  SourceIterator and ToIterator must meet the DefaultCtorEndIterator requirements.

  // primary template; partial specializations *may* be provided
  template <class InputIterator, class FromCharT, template<class> class EndPolicy,
    class ToCharT, template <class, class> class ToIterator = to_iterator,
    template <class, class,
      template<class> class> class SourceIterator = from_iterator>
  class conversion_iterator
    : public ToIterator<SourceIterator<InputIterator, FromCharT, EndPolicy>, ToCharT>
  {
  public:
    typedef SourceIterator<InputIterator, FromCharT, EndPolicy> Source;

    conversion_iterator()
      : ToIterator<Source, ToCharT>()
      {} 

    conversion_iterator(InputIterator begin)
      : ToIterator<Source, ToCharT>(Source(begin))
    {
      static_assert(is_same<typename EndPolicy<InputIterator>::policy_type,
        by_null_policy>::value, "Constructor not valid unless EndPolicy is by_null");
    }

    template <class T>
    conversion_iterator(InputIterator begin, T end,
      // enable_if ensures 2nd argument of 0 is treated as size, not range end
      typename boost::enable_if<boost::is_same<InputIterator, T>, void >::type* x=0)
      : ToIterator<Source, ToCharT>(Source(begin, end))
    {
      static_assert(is_same<typename EndPolicy<InputIterator>::policy_type,
        by_range_policy>::value,"Constructor not valid unless EndPolicy is by_range");
    }

    conversion_iterator(InputIterator begin, std::size_t sz)
      : ToIterator<Source, ToCharT>(Source(begin, sz))
    {
      static_assert(is_same<typename EndPolicy<InputIterator>::policy_type,
        by_size_policy>::value,"Constructor not valid unless EndPolicy is by_size");
    }
  };

//--------------------------------------------------------------------------------------//
//                                  implementation                                      
//--------------------------------------------------------------------------------------//

//------------------------------------  helpers  ---------------------------------------//

namespace detail{

static const ::boost::uint16_t high_surrogate_base = 0xD7C0u;
static const ::boost::uint16_t low_surrogate_base = 0xDC00u;
static const ::boost::uint32_t ten_bit_mask = 0x3FFu;

inline bool is_high_surrogate(::boost::uint16_t v)
{
   return (v & 0xFFFFFC00u) == 0xd800u;
}
inline bool is_low_surrogate(::boost::uint16_t v)
{
   return (v & 0xFFFFFC00u) == 0xdc00u;
}
template <class T>
inline bool is_surrogate(T v)
{
   return (v & 0xFFFFF800u) == 0xd800;
}

inline unsigned utf8_byte_count(boost::uint8_t c)
{
   // if the most significant bit with a zero in it is in position
   // 8-N then there are N bytes in this UTF-8 sequence:
   boost::uint8_t mask = 0x80u;
   unsigned result = 0;
   while(c & mask)
   {
      ++result;
      mask >>= 1;
   }
   return (result == 0) ? 1 : ((result > 4) ? 4 : result);
}

inline unsigned utf8_trailing_byte_count(boost::uint8_t c)
{
   return utf8_byte_count(c) - 1;
}

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4100)
#endif
inline void invalid_utf32_code_point(::boost::uint32_t val)
{
   std::stringstream ss;
   ss << "Invalid UTF-32 code point U+" << std::showbase << std::hex << val << " encountered while trying to encode UTF-16 sequence";
   std::out_of_range e(ss.str());
   BOOST_INTEROP_THROW(e);
}
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

} // namespace detail

//--------------------------------  specializations  -----------------------------------//

//------------------------------  <u8_t> from_iterator  --------------------------------//

  template <class InputIterator, template<class> class EndPolicy>
  class from_iterator<InputIterator, u8_t, EndPolicy>
   : public boost::iterator_facade<from_iterator<InputIterator, u8_t, EndPolicy>,
       u32_t, std::input_iterator_tag, const u32_t>,
     public EndPolicy<InputIterator>
  {
     typedef boost::iterator_facade<from_iterator<InputIterator, u8_t, EndPolicy>,
       u32_t, std::input_iterator_tag, const u32_t> base_type;
     // special values for pending iterator reads:
     BOOST_STATIC_CONSTANT(u32_t, read_pending = 0xffffffffu);

     typedef typename std::iterator_traits<InputIterator>::value_type base_value_type;

     BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 8);
     BOOST_STATIC_ASSERT(sizeof(u32_t)*CHAR_BIT == 32);

     // for the end itorator (i.e. m_is_end == true), other values are unspecified
     InputIterator  m_iterator;  // current position
     mutable u32_t  m_value;     // current value or read_pending
     mutable bool   m_is_end;    // true iff this is the end iterator
    // note: InputIterator is not require to be default construtable [iterator.iterators],
    // so m_iterator == InputIterator() cannot be used to denote the end iterator.

   public:

    // end iterator
    from_iterator() : m_is_end(true) {}  // construct end iterator

    // by_null
    from_iterator(InputIterator begin) : m_iterator(begin) 
    {
      static_assert(is_same<typename EndPolicy<InputIterator>::policy_type,
        by_null_policy>::value,
          "Constructor not valid unless EndPolicy is by_null");
      m_is_end = this->is_end(m_iterator);
      m_value = read_pending;
    }

    // by range
    template <class T>
    from_iterator(InputIterator begin, T end,
      // enable_if ensures 2nd argument of 0 is treated as size, not range end
      typename boost::enable_if<boost::is_same<InputIterator, T>, void >::type* x=0)
      : m_iterator(begin) 
    {
      static_assert(is_same<typename EndPolicy<InputIterator>::policy_type,
        by_range_policy>::value, "Constructor not valid unless EndPolicy is by_range");
      this->end(end);
      m_is_end = this->is_end(m_iterator);
      m_value = read_pending;
    }

    // by_size
    from_iterator(InputIterator begin, std::size_t sz)
      : m_iterator(begin) 
    {
      static_assert(is_same<typename EndPolicy<InputIterator>::policy_type,
        by_size_policy>::value, "Constructor not valid unless EndPolicy is by_size");
      this->size(sz);
      m_is_end = this->is_end(m_iterator);
      m_value = read_pending;
    }

     typename base_type::reference
        dereference() const
     {
        BOOST_ASSERT_MSG(!m_is_end,
          "Attempt to dereference end iterator");
        if (m_value == read_pending)
           extract_current();
        return m_value;
     }

     bool equal(const from_iterator& that) const
     {
       if (m_is_end)
         return that.m_is_end;
       if (that.m_is_end)
         return false;
       return m_iterator == that.m_iterator;
     }

     void increment()
     {
        BOOST_ASSERT_MSG(!m_is_end,
          "Attempt to increment end iterator");
        unsigned count = detail::utf8_byte_count(*m_iterator);
        std::advance(m_iterator, count);
        this->advance(count);
        if (this->is_end(m_iterator))
          m_is_end = true;
        m_value = read_pending;
     }
  private:
     static void invalid_sequence()
     {
        std::out_of_range e(
          "Invalid UTF-8 sequence encountered while trying to encode UTF-32 character");
        BOOST_INTEROP_THROW(e);
     }
     void extract_current()const
     {
        BOOST_ASSERT_MSG(!m_is_end,
          "Internal logic error: extracting from end iterator");
        m_value = static_cast<u32_t>(static_cast< ::boost::uint8_t>(*m_iterator));
        // we must not have a continuation character:
        if((m_value & 0xC0u) == 0x80u)
           invalid_sequence();
        // see how many extra byts we have:
        unsigned extra = detail::utf8_trailing_byte_count(*m_iterator);
        // extract the extra bits, 6 from each extra byte:
        InputIterator next(m_iterator);
        for(unsigned c = 0; c < extra; ++c)
        {
           ++next;
           m_value <<= 6;
           m_value += static_cast<boost::uint8_t>(*next) & 0x3Fu;
        }
        // we now need to remove a few of the leftmost bits, but how many depends
        // upon how many extra bytes we've extracted:
        static const boost::uint32_t masks[4] = 
        {
           0x7Fu,
           0x7FFu,
           0xFFFFu,
           0x1FFFFFu,
        };
        m_value &= masks[extra];
        // check the result:
        if(m_value > static_cast<u32_t>(0x10FFFFu))
           invalid_sequence();
     }
  };

//--------------------------  <u16_t> from_iterator  ---------------------------//

  template <class InputIterator, template<class> class EndPolicy>
  class from_iterator<InputIterator, u16_t, EndPolicy>
   : public boost::iterator_facade<from_iterator<InputIterator, u16_t, EndPolicy>,
       u32_t, std::input_iterator_tag, const u32_t>,
     public EndPolicy<InputIterator>
  {
     typedef boost::iterator_facade<from_iterator<InputIterator, u16_t, EndPolicy>,
       u32_t, std::input_iterator_tag, const u32_t> base_type;
     // special values for pending iterator reads:
     BOOST_STATIC_CONSTANT(u32_t, read_pending = 0xffffffffu);

     typedef typename std::iterator_traits<InputIterator>::value_type base_value_type;

     BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 16);
     BOOST_STATIC_ASSERT(sizeof(u32_t)*CHAR_BIT == 32);

     // for the end itorator (i.e. m_is_end == true), other values are unspecified
     InputIterator  m_iterator;  // current position
     mutable u32_t  m_value;     // current value or read_pending
     mutable bool   m_is_end;    // true iff this is the end iterator
    // note: InputIterator is not require to be default construtable [iterator.iterators],
    // so m_iterator == InputIterator() cannot be used to denote the end iterator.

   public:


    // end iterator
    from_iterator() : m_is_end(true) {} // construct end iterator

    // by_null
    from_iterator(InputIterator begin) : m_iterator(begin) 
    {
      static_assert(is_same<typename EndPolicy<InputIterator>::policy_type,
        by_null_policy>::value, "Constructor not valid unless EndPolicy is by_null");
      m_is_end = this->is_end(m_iterator);
      m_value = read_pending;
    }

    // by range
    template <class T>
    from_iterator(InputIterator begin, T end,
      // enable_if ensures 2nd argument of 0 is treated as size, not range end
      typename boost::enable_if<boost::is_same<InputIterator, T>, void >::type* x=0)
      : m_iterator(begin) 
    {
      static_assert(is_same<typename EndPolicy<InputIterator>::policy_type,
        by_range_policy>::value, "Constructor not valid unless EndPolicy is by_range");
      this->end(end);
      m_is_end = this->is_end(m_iterator);
      m_value = read_pending;
    }

    // by_size
    from_iterator(InputIterator begin, std::size_t sz)
      : m_iterator(begin) 
    {
      static_assert(is_same<typename EndPolicy<InputIterator>::policy_type,
        by_size_policy>::value, "Constructor not valid unless EndPolicy is by_size");
      this->size(sz);
      m_is_end = this->is_end(m_iterator);
      m_value = read_pending;
    }

     typename base_type::reference
        dereference() const
     {
        BOOST_ASSERT_MSG(!m_is_end, 
          "Attempt to dereference end iterator");
        if (m_value == read_pending)
           extract_current();
        return m_value;
     }

     bool equal(const from_iterator& that) const 
     {
       if (m_is_end)
         return that.m_is_end;
       if (that.m_is_end)
         return false;
       return m_iterator == that.m_iterator;
     }

     void increment()
     {
       BOOST_ASSERT_MSG(!m_is_end,
         "Attempt to increment end iterator");
       // skip high surrogate first if there is one:
       if(detail::is_high_surrogate(*m_iterator))
       {
         ++m_iterator;
         this->advance();
       }
       ++m_iterator;
       this->advance();
       if (this->is_end(m_iterator))
          m_is_end = true;
        m_value = read_pending;
     }

  private:
     static void invalid_code_point(::boost::uint16_t val)
     {
        std::stringstream ss;
        ss << "Misplaced UTF-16 surrogate U+" << std::showbase << std::hex << val
           << " encountered while trying to encode UTF-32 sequence";
        std::out_of_range e(ss.str());
        BOOST_INTEROP_THROW(e);
     }
     static void invalid_sequence()
     {
        std::out_of_range e(
          "Invalid UTF-16 sequence encountered while trying to encode UTF-32 character");
        BOOST_INTEROP_THROW(e);
     }
     void extract_current() const
     {
        m_value = static_cast<u32_t>(static_cast< ::boost::uint16_t>(*m_iterator));
        // if the last value is a high surrogate then adjust m_iterator and m_value as needed:
        if(detail::is_high_surrogate(*m_iterator))
        {
           // precondition; next value must have be a low-surrogate:
           InputIterator next(m_iterator);
           u16_t t = *++next;
           if((t & 0xFC00u) != 0xDC00u)
              invalid_code_point(t);
           m_value = (m_value - detail::high_surrogate_base) << 10;
           m_value |= (static_cast<u32_t>(
             static_cast<u16_t>(t)) & detail::ten_bit_mask);
        }
        // postcondition; result must not be a surrogate:
        if(detail::is_surrogate(m_value))
           invalid_code_point(static_cast<u16_t>(m_value));
     }
  };

//---------------------------  <u32_t> from_iterator  --------------------------//

  template <class InputIterator, template<class> class EndPolicy>
  class from_iterator<InputIterator, u32_t, EndPolicy>
    : public boost::iterator_facade<from_iterator<InputIterator, u32_t,
        EndPolicy>, u32_t, std::input_iterator_tag, const u32_t>,
      public EndPolicy<InputIterator>
  {
    // for the end itorator (i.e. m_is_end == true), value of m_iterator unspecified
    InputIterator  m_iterator;
    mutable bool   m_is_end;    // true iff this is the end iterator
    // note: InputIterator is not require to be default construtable [iterator.iterators],
    // so m_iterator == InputIterator() cannot be used to denote the end iterator.

  public:
    from_iterator() : m_is_end(true) {}  // construct end iterator

    // by_null
    from_iterator(InputIterator begin) : m_iterator(begin) 
    {
      static_assert(is_same<typename EndPolicy<InputIterator>::policy_type,
        by_null_policy>::value, "Constructor not valid unless EndPolicy is by_null");
      m_is_end = this->is_end(m_iterator);
    }

    // by range
    template <class T>
    from_iterator(InputIterator begin, T end,
      // enable_if ensures 2nd argument of 0 is treated as size, not range end
      typename boost::enable_if<boost::is_same<InputIterator, T>, void >::type* x=0)
      : m_iterator(begin) 
    {
      static_assert(is_same<typename EndPolicy<InputIterator>::policy_type,
        by_range_policy>::value, "Constructor not valid unless EndPolicy is by_range");
      this->end(end);
      m_is_end = this->is_end(m_iterator);
   }

    // by_size
    from_iterator(InputIterator begin, std::size_t sz)
      : m_iterator(begin) 
    {
      static_assert(is_same<typename EndPolicy<InputIterator>::policy_type,
        by_size_policy>::value, "Constructor not valid unless EndPolicy is by_size");
      this->size(sz);
      m_is_end = this->is_end(m_iterator);
    }

    u32_t dereference() const
    {
      BOOST_ASSERT_MSG(!m_is_end,
        "Attempt to dereference end iterator");
      return *m_iterator;
    }

    bool equal(const from_iterator& that) const
    {
      if (m_is_end)
        return that.m_is_end;
      if (that.m_is_end)
        return false;
      return m_iterator == that.m_iterator;
    }

    void increment()
    {
      BOOST_ASSERT_MSG(!m_is_end,
        "Attempt to increment end iterator");
      ++m_iterator;
      this->advance();
      if (this->is_end(m_iterator))
        m_is_end = true;
    }
  };

//--------------------------  <u8_t> to_iterator  ----------------------------//

  template <class InputIterator>
  class to_iterator<InputIterator, u8_t>
   : public boost::iterator_facade<to_iterator<InputIterator, u8_t>,
       u8_t, std::input_iterator_tag, const u8_t>
  {
     typedef boost::iterator_facade<to_iterator<InputIterator, u8_t>,
       u8_t, std::input_iterator_tag, const u8_t> base_type;
   
     typedef typename std::iterator_traits<InputIterator>::value_type base_value_type;

     BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 32);
     BOOST_STATIC_ASSERT(sizeof(u8_t)*CHAR_BIT == 8);

  public:

     typename base_type::reference
     dereference()const
     {
        if(m_current == 4)
           extract_current();
        return m_values[m_current];
     }
     bool equal(const to_iterator& that)const
     {
        if(m_iterator == that.m_iterator)
        {
           // either the m_current's must be equal, or one must be 0 and 
           // the other 4: which means neither must have bits 1 or 2 set:
           return (m_current == that.m_current)
              || (((m_current | that.m_current) & 3) == 0);
        }
        return false;
     }
     void increment()
     {
        // if we have a pending read then read now, so that we know whether
        // to skip a position, or move to a low-surrogate:
        if(m_current == 4)
        {
           // pending read:
           extract_current();
        }
        // move to the next surrogate position:
        ++m_current;
        // if we've reached the end skip a position:
        if(m_values[m_current] == 0)
        {
           m_current = 4;
           ++m_iterator;
        }
     }

     InputIterator& base() { return m_iterator; }

     // construct:
     to_iterator() : m_iterator(), m_current(0)
     {
        m_values[0] = 0;
        m_values[1] = 0;
        m_values[2] = 0;
        m_values[3] = 0;
        m_values[4] = 0;
     }
     to_iterator(InputIterator b) : m_iterator(b), m_current(4)
     {
        m_values[0] = 0;
        m_values[1] = 0;
        m_values[2] = 0;
        m_values[3] = 0;
        m_values[4] = 0;
        BOOST_XOP_LOG("utf-8 from utf-32");
    }
  private:

     void extract_current()const
     {
        boost::uint32_t c = *m_iterator;
        if(c > 0x10FFFFu)
           detail::invalid_utf32_code_point(c);
        if(c < 0x80u)
        {
           m_values[0] = static_cast<unsigned char>(c);
           m_values[1] = static_cast<unsigned char>(0u);
           m_values[2] = static_cast<unsigned char>(0u);
           m_values[3] = static_cast<unsigned char>(0u);
        }
        else if(c < 0x800u)
        {
           m_values[0] = static_cast<unsigned char>(0xC0u + (c >> 6));
           m_values[1] = static_cast<unsigned char>(0x80u + (c & 0x3Fu));
           m_values[2] = static_cast<unsigned char>(0u);
           m_values[3] = static_cast<unsigned char>(0u);
        }
        else if(c < 0x10000u)
        {
           m_values[0] = static_cast<unsigned char>(0xE0u + (c >> 12));
           m_values[1] = static_cast<unsigned char>(0x80u + ((c >> 6) & 0x3Fu));
           m_values[2] = static_cast<unsigned char>(0x80u + (c & 0x3Fu));
           m_values[3] = static_cast<unsigned char>(0u);
        }
        else
        {
           m_values[0] = static_cast<unsigned char>(0xF0u + (c >> 18));
           m_values[1] = static_cast<unsigned char>(0x80u + ((c >> 12) & 0x3Fu));
           m_values[2] = static_cast<unsigned char>(0x80u + ((c >> 6) & 0x3Fu));
           m_values[3] = static_cast<unsigned char>(0x80u + (c & 0x3Fu));
        }
        m_current= 0;
     }
     InputIterator m_iterator;
     mutable u8_t m_values[5];
     mutable unsigned m_current;
  };

//--------------------------  <u16_t> to_iterator  ---------------------------//

  template <class InputIterator>
  class to_iterator<InputIterator, u16_t>
   : public boost::iterator_facade<to_iterator<InputIterator, u16_t>,
      u16_t, std::input_iterator_tag, const u16_t>
  {
     typedef boost::iterator_facade<to_iterator<InputIterator, u16_t>,
       u16_t, std::input_iterator_tag, const u16_t> base_type;

     typedef typename std::iterator_traits<InputIterator>::value_type base_value_type;

     BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 32);
     BOOST_STATIC_ASSERT(sizeof(u16_t)*CHAR_BIT == 16);

  public:

     typename base_type::reference
     dereference()const
     {
        if(m_current == 2)
           extract_current();
        return m_values[m_current];
     }
     bool equal(const to_iterator& that)const
     {
        if(m_iterator == that.m_iterator)
        {
           // Both m_currents must be equal, or both even
           // this is the same as saying their sum must be even:
           return (m_current + that.m_current) & 1u ? false : true;
        }
        return false;
     }
     void increment()
     {
        // if we have a pending read then read now, so that we know whether
        // to skip a position, or move to a low-surrogate:
        if(m_current == 2)
        {
           // pending read:
           extract_current();
        }
        // move to the next surrogate position:
        ++m_current;
        // if we've reached the end skip a position:
        if(m_values[m_current] == 0)
        {
           m_current = 2;
           ++m_iterator;
        }
     }

     InputIterator& base() {return m_iterator;}

     // construct:
     to_iterator() : m_iterator(), m_current(0)
     {
        m_values[0] = 0;
        m_values[1] = 0;
        m_values[2] = 0;
     }
     to_iterator(InputIterator b) : m_iterator(b), m_current(2)
     {
        m_values[0] = 0;
        m_values[1] = 0;
        m_values[2] = 0;
        BOOST_XOP_LOG("utf-16 from utf-32");
    }
  private:

     void extract_current()const
     {
        // begin by checking for a code point out of range:
        ::boost::uint32_t v = *m_iterator;
        if(v >= 0x10000u)
        {
           if(v > 0x10FFFFu)
              detail::invalid_utf32_code_point(*m_iterator);
           // split into two surrogates:
           m_values[0] = static_cast<u16_t>(v >> 10) + detail::high_surrogate_base;
           m_values[1] = static_cast<u16_t>(v & detail::ten_bit_mask)
             + detail::low_surrogate_base;
           m_current = 0;
           BOOST_ASSERT(detail::is_high_surrogate(m_values[0]));
           BOOST_ASSERT(detail::is_low_surrogate(m_values[1]));
        }
        else
        {
           // 16-bit code point:
           m_values[0] = static_cast<u16_t>(*m_iterator);
           m_values[1] = 0;
           m_current = 0;
           // value must not be a surrogate:
           if(detail::is_surrogate(m_values[0]))
              detail::invalid_utf32_code_point(*m_iterator);
        }
     }
     InputIterator m_iterator;
     mutable u16_t m_values[3];
     mutable unsigned m_current;
  };

//--------------------------  <u32_t> to_iterator  ---------------------------//

  template <class InputIterator>
  class to_iterator<InputIterator, u32_t>
   : public boost::iterator_facade<to_iterator<InputIterator, u32_t>,
      u32_t, std::input_iterator_tag, const u32_t>
  {
    InputIterator m_itr;
  public:
    to_iterator() {}
    to_iterator(InputIterator itr) : m_itr(itr)
    {
      BOOST_XOP_LOG("utf-32 from utf-32");
    }
    u32_t dereference() const { return *m_itr; }
    bool equal(const to_iterator& that) const {return m_itr == that.m_itr;}
    void increment() { ++m_itr; }
    InputIterator& base() {return m_itr;}
  };

//  /***************************************************************************************
// 
//  char and wchar_t codecs need to know the encoding of char and wchar_t strings.
//
//  They need to know how errors are to be reported, such as by exception or by
//  replacement character.
//
//  Since it is totally ineffcient to write separate codecs for generic cases, they need to
//  know a codec sub-type, such as single character, NTBS, etc, and what table to pass to 
//  that subtype.
//
//  Presumably that information comes from global state information. Details to be worked out.
//
//  For this proof-of-concept, the code just uses the tables linked in. The error
//  action for UTF-32 is replacement character '�' (U+FFFD), and for char it is the '?'
//  character U+003F
//
//  **********
//    
//  What happens when a character not in the current T character set is
//  encountered? One possibility to to translate to a replacement character.
//  One of the ASCII characters not normally encountered might be a good choice.
//  The codes below 0x20 or 0x7f (DEL) might be considered. The Wikipedia UTF-8
//  article suggests these are popular:
//   * The replacement character '�' (U+FFFD)
//   * The symbol for substitute '␦' (U+2426) (ISO 2047)
//   * The '?' or '¿' character (U+003F or U+00BF)
//   * The invalid Unicode code points U+DC80..U+DCFF where the low 8 bits are the
//     byte's value.
//   * Interpret the bytes according to another encoding (often ISO-8859-1 or CP1252).
//
//  *************************************************************************************/

//---------------------------  <char> from_iterator  ---------------------------//

  template <class InputIterator, template<class> class EndPolicy>
  class from_iterator<InputIterator, char, EndPolicy>
   : public boost::iterator_facade<from_iterator<InputIterator, char, EndPolicy>,
       u32_t, std::input_iterator_tag, const u32_t>,
     public EndPolicy<InputIterator>
  {
    typedef boost::iterator_facade<from_iterator<InputIterator, char, EndPolicy>,
      u32_t, std::input_iterator_tag, const u32_t> base_type;

    typedef typename std::iterator_traits<InputIterator>::value_type base_value_type;

    BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 8);
    BOOST_STATIC_ASSERT(sizeof(u32_t)*CHAR_BIT == 32);

    // for the end itorator (i.e. m_is_end == true), value of m_iterator unspecified
    InputIterator  m_iterator;
    mutable bool   m_is_end;    // true iff this is the end iterator
    // note: InputIterator is not require to be default construtable [iterator.iterators],
    // so m_iterator == InputIterator() cannot be used to denote the end iterator.

  public:
    // construct:
    from_iterator() : m_is_end(true) {}  // construct end iterator

    // by_null
    from_iterator(InputIterator begin) : m_iterator(begin) 
    {
      static_assert(is_same<typename EndPolicy<InputIterator>::policy_type,
        by_null_policy>::value, "Constructor not valid unless EndPolicy is by_null");
      m_is_end = this->is_end(m_iterator);
    }

    // by range
    template <class T>
    from_iterator(InputIterator begin, T end,
      // enable_if ensures 2nd argument of 0 is treated as size, not range end
      typename boost::enable_if<boost::is_same<InputIterator, T>, void >::type* x=0)
      : m_iterator(begin) 
    {
      static_assert(is_same<typename EndPolicy<InputIterator>::policy_type,
        by_range_policy>::value, "Constructor not valid unless EndPolicy is by_range");
      this->end(end);
      m_is_end = this->is_end(m_iterator);
    }

    // by_size
    from_iterator(InputIterator begin, std::size_t sz)
      : m_iterator(begin) 
    {
      static_assert(is_same<typename EndPolicy<InputIterator>::policy_type,
        by_size_policy>::value, "Constructor not valid unless EndPolicy is by_size");
      this->size(sz);
      m_is_end = this->is_end(m_iterator);
    }

    u32_t dereference() const
    {
      BOOST_ASSERT_MSG(!m_is_end,
        "Attempt to dereference end iterator");
      unsigned char c = static_cast<unsigned char>(*m_iterator);
      return static_cast<u32_t>(interop::detail::to_utf16[c]);
    }

    bool equal(const from_iterator& that) const
    {
      if (m_is_end)
        return that.m_is_end;
      if (that.m_is_end)
        return false;
      return m_iterator == that.m_iterator;
    }

    void increment()
    { 
      BOOST_ASSERT_MSG(!m_is_end,
        "Attempt to increment end iterator");
      ++m_iterator;
      this->advance();
      if (this->is_end(m_iterator))
        m_is_end = true;
    }
  };

//--------------------------  <char> to_iterator  ----------------------------//

#if defined(BOOST_WINDOWS_API) && !defined(BOOST_USE_UTF8)

  template <class InputIterator>
  class to_iterator<InputIterator, char>
   : public boost::iterator_facade<to_iterator<InputIterator, char>,
       char, std::input_iterator_tag, const char>
  {
     typedef boost::iterator_facade<to_iterator<InputIterator, char>,
       char, std::input_iterator_tag, const char> base_type;
   
     typedef typename std::iterator_traits<InputIterator>::value_type base_value_type;

     BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 32);
     BOOST_STATIC_ASSERT(sizeof(char)*CHAR_BIT == 8);

     InputIterator m_iterator;

  public:
    // construct:
    to_iterator() : m_iterator() {}
    to_iterator(InputIterator begin) : m_iterator(begin) {}
    to_iterator(InputIterator begin, std::size_t sz) : m_iterator(begin, sz) {}

    template <class T>
     to_iterator(InputIterator begin, T end,
      // enable_if ensures 2nd argument of 0 is treated as size, not range end
      typename boost::enable_if<boost::is_same<InputIterator, T>, void >::type* x=0)
       : m_iterator(begin, end) {}

    char dereference() const
    {
      BOOST_ASSERT_MSG(m_iterator != InputIterator(),
        "Attempt to dereference end iterator");
      u32_t c = *m_iterator;
      //cout << "*** c is " << hex << c << '\n';
      //cout << "    to_slice[c >> 7] << 7 is "
      //  << unsigned int(interop::detail::slice_index[c >> 7] << 7) << '\n';
      return static_cast<char>(interop::detail::to_char
        [
          (interop::detail::slice_index[c >> 7] << 7) | (c & 0x7f)
        ]);
    }

    bool equal(const to_iterator& that) const
    {
      return m_iterator == that.m_iterator;
    }

    void increment()
    { 
      BOOST_ASSERT_MSG(m_iterator != InputIterator(),
        "Attempt to increment end iterator");
      ++m_iterator;  // may change m_iterator to end iterator
    }

    InputIterator& base() {return m_iterator;}

  };

 # elif defined(BOOST_USE_UTF8) || defined(BOOST_POSIX_API)  // char is UTF-8

  template <class InputIterator>
  class to_iterator<InputIterator, char>
   : public boost::iterator_facade<to_iterator<InputIterator, char>,
       char, std::input_iterator_tag, const char>
  {
     typedef boost::iterator_facade<to_iterator<InputIterator, char>,
       char, std::input_iterator_tag, const char> base_type;
   
     typedef typename std::iterator_traits<InputIterator>::value_type base_value_type;

     BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 32);
     BOOST_STATIC_ASSERT(sizeof(char)*CHAR_BIT == 8);

  public:

     typename base_type::reference
     dereference()const
     {
        if(m_current == 4)
           extract_current();
        return m_values[m_current];
     }
     bool equal(const to_iterator& that)const
     {
        if(m_iterator == that.m_iterator)
        {
           // either the m_current's must be equal, or one must be 0 and 
           // the other 4: which means neither must have bits 1 or 2 set:
           return (m_current == that.m_current)
              || (((m_current | that.m_current) & 3) == 0);
        }
        return false;
     }
     void increment()
     {
        // if we have a pending read then read now, so that we know whether
        // to skip a position, or move to a low-surrogate:
        if(m_current == 4)
        {
           // pending read:
           extract_current();
        }
        // move to the next surrogate position:
        ++m_current;
        // if we've reached the end skip a position:
        if(m_values[m_current] == 0)
        {
           m_current = 4;
           ++m_iterator;
        }
     }

     InputIterator& base() { return m_iterator; }

     // construct:
     to_iterator() : m_iterator(), m_current(0)
     {
        m_values[0] = 0;
        m_values[1] = 0;
        m_values[2] = 0;
        m_values[3] = 0;
        m_values[4] = 0;
     }
     to_iterator(InputIterator b) : m_iterator(b), m_current(4)
     {
        m_values[0] = 0;
        m_values[1] = 0;
        m_values[2] = 0;
        m_values[3] = 0;
        m_values[4] = 0;
        BOOST_XOP_LOG("char from utf-32");
    }
  private:

     void extract_current()const
     {
        boost::uint32_t c = *m_iterator;
        if(c > 0x10FFFFu)
           detail::invalid_utf32_code_point(c);
        if(c < 0x80u)
        {
           m_values[0] = static_cast<char>(c);
           m_values[1] = static_cast<char>(0u);
           m_values[2] = static_cast<char>(0u);
           m_values[3] = static_cast<char>(0u);
        }
        else if(c < 0x800u)
        {
           m_values[0] = static_cast<char>(0xC0u + (c >> 6));
           m_values[1] = static_cast<char>(0x80u + (c & 0x3Fu));
           m_values[2] = static_cast<char>(0u);
           m_values[3] = static_cast<char>(0u);
        }
        else if(c < 0x10000u)
        {
           m_values[0] = static_cast<char>(0xE0u + (c >> 12));
           m_values[1] = static_cast<char>(0x80u + ((c >> 6) & 0x3Fu));
           m_values[2] = static_cast<char>(0x80u + (c & 0x3Fu));
           m_values[3] = static_cast<char>(0u);
        }
        else
        {
           m_values[0] = static_cast<char>(0xF0u + (c >> 18));
           m_values[1] = static_cast<char>(0x80u + ((c >> 12) & 0x3Fu));
           m_values[2] = static_cast<char>(0x80u + ((c >> 6) & 0x3Fu));
           m_values[3] = static_cast<char>(0x80u + (c & 0x3Fu));
        }
        m_current= 0;
     }
     InputIterator m_iterator;
     mutable char m_values[5];
     mutable unsigned m_current;
  };

# else
#   error Sorry, not implemented for other than 16 or 32 bit wchar_t
# endif

//-------------------------  <wchar_t> from_iterator  --------------------------//

#if defined(BOOST_WINDOWS_API)  // assume wchar_t is UTF-16

namespace detail
{
  template <class InputIterator>
  class wide_to_u16
    : public boost::iterator_facade<wide_to_u16<InputIterator>,
       u16_t, std::input_iterator_tag, const u16_t>
  {
    InputIterator m_itr;
  public:
    wide_to_u16() {}
    explicit wide_to_u16(InputIterator itr) : m_itr(itr) {}
    u16_t dereference() const {return u16_t(*m_itr);}
    bool equal(const wide_to_u16& that) const {return m_itr == that.m_itr;}
    void increment() {++m_itr;}
  };
}
  BOOST_STATIC_ASSERT(sizeof(wchar_t)*CHAR_BIT == 16);

  template <class InputIterator, template<class> class EndPolicy>
  class from_iterator<InputIterator, wchar_t, EndPolicy>
   : public boost::iterator_facade<from_iterator<InputIterator, wchar_t, EndPolicy>,
       u32_t, std::input_iterator_tag, const u32_t>
  {
     typedef boost::iterator_facade<from_iterator<InputIterator, wchar_t, EndPolicy>,
       u32_t, std::input_iterator_tag, const u32_t> base_type;

     typedef typename std::iterator_traits<InputIterator>::value_type base_value_type;

     BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 16);
     BOOST_STATIC_ASSERT(sizeof(u32_t)*CHAR_BIT == 32);

     from_iterator<detail::wide_to_u16<InputIterator>, u16_t, EndPolicy> m_iterator;

  public:
     from_iterator() : m_iterator() {}
     from_iterator(InputIterator b) : m_iterator(detail::wide_to_u16<InputIterator>(b))
     {
       BOOST_XOP_LOG("wchar_t to utf-32");
     }
     u32_t dereference() const { return *m_iterator; }
     bool equal(const from_iterator& that) const {return m_iterator == that.m_iterator;}
     void increment() { ++m_iterator; }
     void size(std::size_t sz) { m_iterator.size(sz); }
     void end(InputIterator e) { m_iterator.end(detail::wide_to_u16<InputIterator>(e)); }
  };

# elif defined(BOOST_POSIX_API)  // POSIX; assumes wchar_t is UTF-32

  BOOST_STATIC_ASSERT(sizeof(wchar_t)*CHAR_BIT == 32);
  //...
# else
#   error Sorry, not implemented for other than 16 or 32 bit wchar_t
# endif

//-------------------------  <wchar_t> to_iterator  --------------------------//

#if defined(BOOST_WINDOWS_API)  // assume wchar_t is UTF-16

  BOOST_STATIC_ASSERT(sizeof(wchar_t)*CHAR_BIT == 16);

  template <class InputIterator>
  class to_iterator<InputIterator, wchar_t>
   : public boost::iterator_facade<to_iterator<InputIterator, wchar_t>,
       wchar_t, std::input_iterator_tag, const wchar_t>
  {
     to_iterator<InputIterator, u16_t>  m_iterator;

  public:
     wchar_t dereference() const
     {
       return static_cast<wchar_t>(*m_iterator);
     }

     bool equal(const to_iterator& that) const
     {
       return m_iterator == that.m_iterator;
     }

     void increment()  { ++m_iterator; }

     InputIterator& base() {return m_iterator.base();}

     // construct:
     to_iterator() : m_iterator() {}
     to_iterator(InputIterator b) : m_iterator(b)
     {
       BOOST_XOP_LOG("wchar_t from utf-32");
     }
  };

# elif defined(BOOST_POSIX_API)  // POSIX; assumes wchar_t is UTF-32

  BOOST_STATIC_ASSERT(sizeof(wchar_t)*CHAR_BIT == 32);
  //...
# else
#   error Sorry, not implemented for other than 16 or 32 bit wchar_t
# endif

//------------------------------------  from_utf8  ---------------------------------------//

  //template <class InputIterator, template<class> class EndPolicy>  
  ////  iterator_traits<InputIterator>::value_type must be char
  //class from_utf8;  // value_type is c32_t

//-------------------------------------  to_utf8  ----------------------------------------//

  template <class InputIterator, class ToCharT>
  //  iterator_traits<InputIterator>::value_type must be c32_t
  //  ToCharT must be char
  class to_utf8  // value_type is char
   : public boost::iterator_facade<to_utf8<InputIterator, ToCharT>,
       char, std::input_iterator_tag, const char>
  {
     typedef boost::iterator_facade<to_utf8<InputIterator, ToCharT>,
       char, std::input_iterator_tag, const char> base_type;
   
     typedef typename std::iterator_traits<InputIterator>::value_type base_value_type;

     BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 32);
     BOOST_STATIC_ASSERT(sizeof(char)*CHAR_BIT == 8);

     InputIterator m_iterator;
     mutable char m_values[5];
     mutable unsigned m_current;

  public:

     // construct:
     to_utf8() : m_iterator(), m_current(0)
     {
        m_values[0] = 0;
        m_values[1] = 0;
        m_values[2] = 0;
        m_values[3] = 0;
        m_values[4] = 0;
     }
     to_utf8(InputIterator begin) : m_iterator(begin), m_current(4)
     {
        m_values[0] = 0;
        m_values[1] = 0;
        m_values[2] = 0;
        m_values[3] = 0;
        m_values[4] = 0;
std::cout << "to_utf8 constructor" << std::endl;
    }

     typename base_type::reference
     dereference()const
     {
        if(m_current == 4)
           extract_current();
        return m_values[m_current];
     }
     bool equal(const to_utf8& that)const
     {
        if(m_iterator == that.m_iterator)
        {
           // either the m_current's must be equal, or one must be 0 and 
           // the other 4: which means neither must have bits 1 or 2 set:
           return (m_current == that.m_current)
              || (((m_current | that.m_current) & 3) == 0);
        }
        return false;
     }
     void increment()
     {
        // if we have a pending read then read now, so that we know whether
        // to skip a position, or move to a low-surrogate:
        if(m_current == 4)
        {
           // pending read:
           extract_current();
        }
        // move to the next surrogate position:
        ++m_current;
        // if we've reached the end skip a position:
        if(m_values[m_current] == 0)
        {
           m_current = 4;
           ++m_iterator;
        }
     }

     InputIterator& base() { return m_iterator; }

  private:

     void extract_current()const
     {
        boost::uint32_t c = *m_iterator;
        if(c > 0x10FFFFu)
           detail::invalid_utf32_code_point(c);
        if(c < 0x80u)
        {
           m_values[0] = static_cast<char>(c);
           m_values[1] = static_cast<char>(0u);
           m_values[2] = static_cast<char>(0u);
           m_values[3] = static_cast<char>(0u);
        }
        else if(c < 0x800u)
        {
           m_values[0] = static_cast<char>(0xC0u + (c >> 6));
           m_values[1] = static_cast<char>(0x80u + (c & 0x3Fu));
           m_values[2] = static_cast<char>(0u);
           m_values[3] = static_cast<char>(0u);
        }
        else if(c < 0x10000u)
        {
           m_values[0] = static_cast<char>(0xE0u + (c >> 12));
           m_values[1] = static_cast<char>(0x80u + ((c >> 6) & 0x3Fu));
           m_values[2] = static_cast<char>(0x80u + (c & 0x3Fu));
           m_values[3] = static_cast<char>(0u);
        }
        else
        {
           m_values[0] = static_cast<char>(0xF0u + (c >> 18));
           m_values[1] = static_cast<char>(0x80u + ((c >> 12) & 0x3Fu));
           m_values[2] = static_cast<char>(0x80u + ((c >> 6) & 0x3Fu));
           m_values[3] = static_cast<char>(0x80u + (c & 0x3Fu));
        }
        m_current= 0;
     }
  };

  //---------------------------------  policy_iterator  ----------------------------------//

  template <class InputIterator, template<class> class EndPolicy>
  class policy_iterator
    : public boost::iterator_facade<policy_iterator<InputIterator, EndPolicy>,
        typename std::iterator_traits<InputIterator>::value_type,
        std::input_iterator_tag,
        const typename std::iterator_traits<InputIterator>::value_type>,
      public EndPolicy<InputIterator>

  {
    InputIterator m_itr;
  public:
    policy_iterator() {}
    explicit policy_iterator(InputIterator itr) : m_itr(itr) {}
    typename std::iterator_traits<InputIterator>::value_type dereference() const
    {
      if (this->is_end(m_itr))
        return 0;
      return *m_itr;
    }
    bool equal(const policy_iterator& that) const {return m_itr == that.m_itr;}
    void increment()
    {
      BOOST_ASSERT_MSG(!this->is_end(m_itr), "Attempt to increment past end");
      ++m_itr;
      this->advance();
    }
  };

}  // namespace interop

}  // namespace boost

#endif  // BOOST_INTEROP_ITERATOR_ADAPTER_HPP