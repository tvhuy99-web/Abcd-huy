#include "cata_bitset.h"

#include "cata_assert.h"

namespace
{

size_t bit_count_to_block_count( size_t bits )
{
    return ( bits + tiny_bitset::kBitsPerBlock - 1 ) / tiny_bitset::kBitsPerBlock;
}

}

void tiny_bitset::resize_heap( size_t requested_bits ) noexcept
{
    size_t blocks_needed = bit_count_to_block_count( requested_bits );



    blocks_needed = ( ( blocks_needed + ( kMinimumBlockPerAlloc - 1 ) ) / kMinimumBlockPerAlloc ) *
                    kMinimumBlockPerAlloc + 1;

    if( is_inline() ) {

        block_t *data = static_cast<block_t *>( malloc( sizeof( block_t ) * ( blocks_needed + 1 ) ) );
        cata_assert( data != nullptr );

        data[ 0 ] = requested_bits;

        data[ 1 ] = storage_ & ~kMetaMask;
        set_storage( data + 1 );
    } else {
        block_t *data = static_cast<block_t *>( realloc( real_heap_allocation(),
                                                sizeof( block_t ) * ( blocks_needed + 1 ) ) );
        if( data != real_heap_allocation() ) {
            set_storage( data + 1 );
        }
    }

    set_size( requested_bits );
}

void tiny_bitset::set_storage( block_t *data )
{
    // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
    memcpy( &storage_, &data, sizeof( data ) );
}
