/* Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#pragma once

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <mira/identity.hpp>
#include <mira/indexed_by.hpp>
#include <mira/ordered_index_fwd.hpp>
#include <mira/composite_key_fwd.hpp>
#include <mira/slice_pack.hpp>

#include <rocksdb/db.h>

#include <memory>

#define ID_INDEX 1

namespace mira{

namespace multi_index{

/* Default value for IndexSpecifierList specifies a container
 * equivalent to std::set<Value>.
 */

template<
  typename Value,
  typename Serializer,
  typename IndexSpecifierList=indexed_by<ordered_unique<identity<Value>>>>
class multi_index_container;

template<typename MultiIndexContainer,int N>
struct nth_index;

template<typename MultiIndexContainer,typename Tag>
struct index;

template<typename MultiIndexContainer,int N>
struct nth_index_iterator;

template<typename MultiIndexContainer,int N>
struct nth_index_const_iterator;

template<typename MultiIndexContainer,typename Tag>
struct index_iterator;

template<typename MultiIndexContainer,typename Tag>
struct index_const_iterator;

/* get and project functions not fwd declared due to problems
 * with dependent typenames
 */

template<
  typename Value1,typename Serializer1,typename IndexSpecifierList1,
  typename Value2,typename Serializer2,typename IndexSpecifierList2
>
bool operator==(
  const multi_index_container<Value1,Serializer1,IndexSpecifierList1>& x,
  const multi_index_container<Value2,Serializer2,IndexSpecifierList2>& y);

template<
  typename Value1,typename Serializer1,typename IndexSpecifierList1,
  typename Value2,typename Serializer2,typename IndexSpecifierList2
>
bool operator<(
  const multi_index_container<Value1,Serializer1,IndexSpecifierList1>& x,
  const multi_index_container<Value2,Serializer2,IndexSpecifierList2>& y);

template<
  typename Value1,typename Serializer1,typename IndexSpecifierList1,
  typename Value2,typename Serializer2,typename IndexSpecifierList2
>
bool operator!=(
  const multi_index_container<Value1,Serializer1,IndexSpecifierList1>& x,
  const multi_index_container<Value2,Serializer2,IndexSpecifierList2>& y);

template<
  typename Value1,typename Serializer1,typename IndexSpecifierList1,
  typename Value2,typename Serializer2,typename IndexSpecifierList2
>
bool operator>(
  const multi_index_container<Value1,Serializer1,IndexSpecifierList1>& x,
  const multi_index_container<Value2,Serializer2,IndexSpecifierList2>& y);

template<
  typename Value1,typename Serializer1,typename IndexSpecifierList1,
  typename Value2,typename Serializer2,typename IndexSpecifierList2
>
bool operator>=(
  const multi_index_container<Value1,Serializer1,IndexSpecifierList1>& x,
  const multi_index_container<Value2,Serializer2,IndexSpecifierList2>& y);

template<
  typename Value1,typename Serializer1,typename IndexSpecifierList1,
  typename Value2,typename Serializer2,typename IndexSpecifierList2
>
bool operator<=(
  const multi_index_container<Value1,Serializer1,IndexSpecifierList1>& x,
  const multi_index_container<Value2,Serializer2,IndexSpecifierList2>& y);

template<typename Value,typename Serializer,typename IndexSpecifierList>
void swap(
  multi_index_container<Value,Serializer,IndexSpecifierList>& x,
  multi_index_container<Value,Serializer,IndexSpecifierList>& y);

typedef std::shared_ptr< ::rocksdb::DB >                 db_ptr;
typedef std::vector< ::rocksdb::ColumnFamilyDescriptor > column_definitions;
typedef std::vector< std::shared_ptr< ::rocksdb::ColumnFamilyHandle > >    column_handles;

} /* namespace multi_index */

/* multi_index_container, being the main type of this library, is promoted to
 * namespace mira.
 */

using multi_index::multi_index_container;

} /* namespace mira */
