#pragma once

#include <vector>
#include <functional>
#include <boost/test/unit_test.hpp>

template < typename IndexType, typename Object, typename Order >
void basic_test( const std::vector< uint64_t >& v,
                 std::function< void( Object& ) > call,
                 IndexType& index )
{
   const auto& c = index.template get< Order >();

   BOOST_TEST_MESSAGE( "Creating `v.size()` objects" );
   for( const auto& item : v )
   {
      Object o;
      o.id = item;
      call( o );
      o.val = 100 - item;

      index.emplace( std::move( o ) );
   }

   BOOST_REQUIRE( v.size() == c.size() );

   BOOST_TEST_MESSAGE( "Removing all objects" );
   {
      auto it = c.begin();
      while ( it != c.end() )
      {
         auto to_remove = it; ++it;
         index.erase( to_remove );
      }
   }

   BOOST_REQUIRE( c.size() == 0 );

   BOOST_TEST_MESSAGE( "Creating 1 object" );
   {
      Object o;
      o.id = 0;
      call( o );
      o.val = 888;

      index.emplace( std::move( o ) );
   }

   BOOST_TEST_MESSAGE( "Modyfing 1 object" );
   index.modify( c.begin(), [] ( Object& o )
   {
      o.val = o.val + 1;
   } );

   BOOST_REQUIRE( c.size() == 1 );

   BOOST_TEST_MESSAGE( "Removing 1 object" );
   index.erase( c.begin() );

   BOOST_REQUIRE( c.size() == 0 );

   BOOST_TEST_MESSAGE( "Creating `v.size()` objects" );
   for( const auto& item : v )
   {
      Object o;
      o.id = item;
      call( o );
      o.val = 100 - item;

      index.emplace( std::move( o ) );
   }

   BOOST_REQUIRE( v.size() == c.size() );

   BOOST_TEST_MESSAGE( "Removing all objects one by one" );
   {
      auto it = c.begin();
      while (it != c.end())
      {
         auto to_remove = it; ++it;
         index.erase( to_remove );
      }
   }

   BOOST_REQUIRE( c.size() == 0 );
}

template < typename IndexType, typename Object, typename Order >
void insert_remove_test( const std::vector< uint64_t >& v,
                         std::function< void( Object& ) > call,
                         IndexType& index )
{
   const auto& c = index.template get< Order >();

   int64_t cnt = 0;

   BOOST_TEST_MESSAGE( "Creating `v.size()` objects" );
   for( const auto& item : v )
   {
      Object o;
      o.id = item;
      call( o );
      o.val = item;

      index.emplace( std::move( o ) );

      ++cnt;
   }
   BOOST_REQUIRE( v.size() == c.size() );

   BOOST_TEST_MESSAGE( "Removing objects one by one. Every object is removed by erase method" );
   for( int64_t i = 0; i < cnt; ++i )
   {
      auto found = c.find( i );
      BOOST_REQUIRE( found != c.end() );

      index.erase( found );
      BOOST_REQUIRE( v.size() - i - 1 == c.size() );
   }
   BOOST_REQUIRE( c.size() == 0 );

   BOOST_TEST_MESSAGE( "Creating 2 objects" );
   cnt = 0;
   for( const auto& item : v )
   {
      Object o;
      o.id = item;
      call( o );
      o.val = cnt;

      index.emplace( std::move( o ) );

      if( ++cnt == 2 )
         break;
   }
   BOOST_REQUIRE( c.size() == 2 );

   BOOST_TEST_MESSAGE( "Removing first object" );
   index.erase( c.begin() );

   BOOST_REQUIRE( c.size() == 1 );

   BOOST_TEST_MESSAGE( "Adding object with id_key = 0" );
   Object o;
   o.id = v[0];
   call( o );
   o.val = 100;

   index.emplace( std::move( o ) );

   BOOST_REQUIRE( c.size() == 2 );

   BOOST_TEST_MESSAGE( "Remove second object" );
   auto found = c.end();
   --found;
   //auto first = std::prev( found, 1 ); // This crashes
   /* This alternative works */
   auto first = found;
   --first;

   index.erase( found );
   BOOST_REQUIRE( c.size() == 1 );

   index.erase( first );
   BOOST_REQUIRE( c.size() == 0 );
}

template < typename IndexType, typename Object, typename Order >
void insert_remove_collision_test( const std::vector< uint64_t >& v,
                                   std::function< void( Object& ) > call1,
                                   std::function< void( Object& ) > call2,
                                   std::function< void( Object& ) > call3,
                                   std::function< void( Object& ) > call4,
                                   IndexType& index )
{
   const auto& c = index.template get< Order >();

   BOOST_TEST_MESSAGE( "Adding 2 objects with collision - the same id" );

   {
      Object o;
      call1( o );
      index.emplace( std::move( o ) );
   }

   {
      Object o;
      call2( o );
      BOOST_CHECK( index.emplace( std::move( o ) ).second == false );
   }

   BOOST_REQUIRE( c.size() == 1 );

   BOOST_TEST_MESSAGE( "Removing object and adding 2 objects with collision - the same name+val" );
   index.erase( c.begin() );

   {
      Object o;
      call3( o );
      index.emplace( std::move( o ) );
   }

   {
      Object o;
      call4( o );
      BOOST_CHECK( index.emplace( std::move( o ) ).second == false );
   }

   BOOST_REQUIRE( c.size() == 1 );
}

template < typename IndexType, typename Object, typename Order >
void modify_test( const std::vector< uint64_t >& v,
                  std::function< void( Object& ) > call1,
                  std::function< void( Object& ) > call2,
                  std::function< void( const Object& ) > call3,
                  std::function< void( const Object& ) > call4,
                  std::function< void( bool ) > call5,
                  IndexType& index )
{
   const auto& c = index.template get< Order >();

   BOOST_TEST_MESSAGE( "Creating `v.size()` objects" );
   for( const auto& item : v )
   {
      Object o;
      o.id = item;
      call1( o );
      o.val = item + 100;

      index.emplace( std::move( o ) );
   }
   BOOST_REQUIRE( v.size() == c.size() );

   BOOST_TEST_MESSAGE( "Modifying key in all objects" );
   auto it = c.begin();
   auto end = c.end();
   for( ; it != end; ++it )
      index.modify( it, call2 );

   BOOST_REQUIRE( v.size() == c.size() );
   int32_t cnt = 0;
   for( const auto& item : c )
   {
      BOOST_REQUIRE( item.id == v[ cnt++ ] );
      index.modify( index.iterator_to( item ), call3 );
      index.modify( index.iterator_to( item ), call4 );
   }

   BOOST_TEST_MESSAGE( "Modifying 'val' key in order to create collisions for all objects" );
   uint32_t first_val = c.begin()->val;
   it = std::next( c.begin(), 1 );
   for( ; it != end; ++it )
   {
      bool result = index.modify( it, [&] ( Object& o )
      {
         o.val = first_val;
      });

      call5( result );
   }
}

template < typename IndexType, typename Object, typename SimpleIndex, typename ComplexIndex >
void misc_test( const std::vector< uint64_t >& v, IndexType& index )
{
   const auto& c = index.template get< SimpleIndex >();

   BOOST_TEST_MESSAGE( "Creating `v.size()` objects" );
   for( const auto& item : v )
   {
      Object o;
      o.id = item;
      o.name = "any_name";
      o.val = item + 200;

      index.emplace( std::move( o ) );
   }
   BOOST_REQUIRE( v.size() == c.size() );

   BOOST_TEST_MESSAGE( "Reading all elements" );
   uint32_t cnt = 0;
   for( const auto& item : c )
   {
      BOOST_REQUIRE( item.id == v[ cnt++ ] );
      BOOST_REQUIRE( item.name == "any_name" );
      BOOST_REQUIRE( item.val == item.id + 200 );
   }

   BOOST_TEST_MESSAGE( "Removing 2 objects: first and last" );
   index.erase( c.begin() );
   {
      auto last = c.end();
      --last;
      index.erase( last );
   }

   BOOST_REQUIRE( v.size() - 2 == c.size() );

   //Modyfing key `name` in one object.
   index.modify( c.begin(), [] ( Object& o ) { o.name = "completely_different_name"; } );
   auto it1 = c.begin();
   BOOST_REQUIRE( it1->name == "completely_different_name" );
   uint32_t val1 = it1->val;

   //Creating collision in two objects: [1] and [2]
   auto it2 = it1;
   it2++;
   BOOST_REQUIRE( index.modify( it2, [ val1 ]( Object& obj ){ obj.name = "completely_different_name"; obj.val = val1;} ) == false );

   //Removing object [1].
   it1 = c.begin();
   it1++;

   index.erase( it1 );
   BOOST_REQUIRE( v.size() - 3 == c.size() );

   //Removing all remaining objects
   auto it_erase = c.begin();
   while( it_erase != c.end() )
   {
      index.erase( it_erase );
      it_erase = c.begin();
   }
   BOOST_REQUIRE( c.size() == 0 );

   cnt = 0;
   BOOST_TEST_MESSAGE( "Creating `v.size()` objects. There are 'v.size() - 1' collisions." );
   for( const auto& item : v )
   {
      auto constructor = [ &item ]( Object &obj )
      {
         obj.id = item;
         obj.name = "all_objects_have_the_same_name";
         obj.val = 667;
      };

      if( cnt == 0 )
      {
         Object o;
         constructor( o );
         index.emplace( std::move( o ) );
      }
      else
      {
         Object o;
         constructor( o );
         BOOST_CHECK( index.emplace( std::move( o ) ).second == false );
      }

      cnt++;
   }
   BOOST_REQUIRE( c.size() == 1 );

   auto it_only_one = c.begin();
   BOOST_REQUIRE( it_only_one->id == v[0] );
   BOOST_REQUIRE( it_only_one->name == "all_objects_have_the_same_name" );
   BOOST_REQUIRE( it_only_one->val == 667 );

   BOOST_TEST_MESSAGE( "Erasing one objects." );

   index.erase( it_only_one );
   BOOST_REQUIRE( c.size() == 0 );

   cnt = 0;
   BOOST_TEST_MESSAGE( "Creating `v.size()` objects." );
   for( const auto& item : v )
   {
      Object o;
      o.id = item;
      o.name = "object nr:" + std::to_string( cnt++ );
      o.val = 5000;

      index.emplace( std::move( o ) );
   }
   BOOST_REQUIRE( c.size() == v.size() );

   BOOST_TEST_MESSAGE( "Finding some objects according to given index." );

   const auto& ordered_idx = index.template get< SimpleIndex >();
   const auto& composite_ordered_idx = index.template get< ComplexIndex >();

   auto found = ordered_idx.find( v.size() - 1 );
   BOOST_REQUIRE( found != ordered_idx.end() );
   BOOST_REQUIRE( found->id == v.size() - 1 );

   found = ordered_idx.find( 987654 );
   BOOST_REQUIRE( found == ordered_idx.end() );

   found = ordered_idx.find( v[0] );
   BOOST_REQUIRE( found != ordered_idx.end() );

   auto cfound = composite_ordered_idx.find( boost::make_tuple( "stupid_name", 5000 ) );
   BOOST_REQUIRE( cfound == composite_ordered_idx.end() );

   cfound = composite_ordered_idx.find( boost::make_tuple( "object nr:" + std::to_string( 0 ), 5000 ) );
   auto copy_cfound = cfound;
   auto cfound2 = composite_ordered_idx.find( boost::make_tuple( "object nr:" + std::to_string( 9 ), 5000 ) );
   BOOST_REQUIRE( cfound == composite_ordered_idx.begin() );

   {
      auto last = composite_ordered_idx.end();
      --last;
      BOOST_REQUIRE( cfound2 == last );
   }

   cnt = 0;
   for ( auto it = cfound; it != composite_ordered_idx.end(); ++it )
      cnt++;
   BOOST_REQUIRE( cnt == v.size() );

   BOOST_TEST_MESSAGE( "Removing all data using iterators from 'ComplexIndex' index");
   while( copy_cfound != composite_ordered_idx.end() )
   {
      auto tmp = copy_cfound;
      ++copy_cfound;
      index.erase( index.iterator_to( *tmp ) );
   }
   BOOST_REQUIRE( c.size() == 0 );
}

template < typename IndexType, typename Object, typename SimpleIndex, typename ComplexIndex, typename AnotherComplexIndex >
void misc_test3( const std::vector< uint64_t >& v, IndexType& index )
{
   const auto& c = index.template get< SimpleIndex >();

   BOOST_TEST_MESSAGE( "Creating `v.size()` objects" );
   for ( const auto& item : v )
   {
      Object obj;
      obj.id = item;
      obj.val = item + 1;
      obj.val2 = item + 2;
      obj.val3 = item + 3;

      index.emplace( std::move( obj ) );
   }
   BOOST_REQUIRE( v.size() == c.size() );

   BOOST_TEST_MESSAGE( "Finding some objects according to given index." );
   const auto& ordered_idx = index.template get< SimpleIndex >();
   const auto& composite_ordered_idx = index.template get< ComplexIndex >();
   const auto& another_composite_ordered_idx = index.template get< AnotherComplexIndex >();

   auto found = ordered_idx.lower_bound( 5739854 );
   BOOST_REQUIRE( found == ordered_idx.end() );

   found = ordered_idx.upper_bound( 5739854 );
   BOOST_REQUIRE( found == ordered_idx.end() );

   found = ordered_idx.lower_bound(  ordered_idx.begin()->id );
   BOOST_REQUIRE( found == ordered_idx.begin() );

   auto found2 = ordered_idx.upper_bound( ordered_idx.begin()->id );
   BOOST_REQUIRE( found == --found2 );

   auto cfound = composite_ordered_idx.find( boost::make_tuple( 667, 5000 ) );
   BOOST_REQUIRE( cfound == composite_ordered_idx.end() );

   cfound = composite_ordered_idx.find( boost::make_tuple( 0 + 1, 0 + 2 ) );
   BOOST_REQUIRE( cfound != composite_ordered_idx.end() );

   auto another_cfound = another_composite_ordered_idx.find( boost::make_tuple( 0 + 1, 0 + 3 ) );
   BOOST_REQUIRE( another_cfound != another_composite_ordered_idx.end() );

   BOOST_TEST_MESSAGE( "Removing 1 object using iterator from 'AnotherComplexIndex' index");
   index.erase( index.iterator_to( *another_cfound ) );
   BOOST_REQUIRE( c.size() == v.size() - 1 );

   another_cfound = another_composite_ordered_idx.find( boost::make_tuple( 0 + 1, 0 + 3 ) );
   BOOST_REQUIRE( another_cfound == another_composite_ordered_idx.end() );
}
