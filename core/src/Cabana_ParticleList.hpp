/****************************************************************************
 * Copyright (c) 2018-2022 by the Cabana authors                            *
 * All rights reserved.                                                     *
 *                                                                          *
 * This file is part of the Cabana library. Cabana is distributed under a   *
 * BSD 3-clause license. For the licensing terms see the LICENSE file in    *
 * the top-level directory.                                                 *
 *                                                                          *
 * SPDX-License-Identifier: BSD-3-Clause                                    *
 ****************************************************************************/

/*!
  \file Cabana_ParticleList.hpp
  \brief Application-level particle storage and single particle access.
*/
#ifndef CABANA_PARTICLELIST_HPP
#define CABANA_PARTICLELIST_HPP

#include <Cabana_AoSoA.hpp>
#include <Cabana_Fields.hpp>
#include <Cabana_MemberTypes.hpp>
#include <Cabana_SoA.hpp>
#include <Cabana_Tuple.hpp>

#include <memory>
#include <string>
#include <type_traits>

namespace Cabana
{
//---------------------------------------------------------------------------//
//! Extract AoSoA particle fields for ParticleList.
template <class... FieldTags>
struct ParticleTraits
{
    //! AoSoA particle fields.
    using member_types = Cabana::MemberTypes<typename FieldTags::data_type...>;
};

//---------------------------------------------------------------------------//
//! Single particle copy. Wraps a tuple copy of a particle.
template <class... FieldTags>
struct Particle
{
    //! Particle AoSoA member types.
    using traits = ParticleTraits<FieldTags...>;
    //! Particle tuple type.
    using tuple_type = Cabana::Tuple<typename traits::member_types>;
    //! Vector length (for compatibility with ParticleView).
    static constexpr int vector_length = 1;

    //! Default constructor.
    Particle() = default;

    //! Tuple wrapper constructor.
    KOKKOS_FORCEINLINE_FUNCTION
    Particle( const tuple_type& tuple )
        : _tuple( tuple )
    {
    }

    //! Get the underlying tuple.
    KOKKOS_FORCEINLINE_FUNCTION
    tuple_type& tuple() { return _tuple; }

    //! Get the underlying tuple (const).
    KOKKOS_FORCEINLINE_FUNCTION
    const tuple_type& tuple() const { return _tuple; }

    //! The tuple this particle wraps.
    tuple_type _tuple;
};

//---------------------------------------------------------------------------//
//! Single SoA particle view. Wraps a view of the SoA the particle resides in.
template <int VectorLength, class... FieldTags>
struct ParticleView
{
    //! Particle AoSoA member types.
    using traits = ParticleTraits<FieldTags...>;
    //! Particle SoA type.
    using soa_type = Cabana::SoA<typename traits::member_types, VectorLength>;
    //! AoSoA vector length
    static constexpr int vector_length = VectorLength;

    //! Default constructor.
    ParticleView() = default;

    //! Tuple wrapper constructor.
    KOKKOS_FORCEINLINE_FUNCTION
    ParticleView( soa_type& soa, const int vector_index )
        : _soa( soa )
        , _vector_index( vector_index )
    {
    }

    //! Get the underlying SoA.
    KOKKOS_FORCEINLINE_FUNCTION
    soa_type& soa() { return _soa; }

    //! Get the underlying SoA (const).
    KOKKOS_FORCEINLINE_FUNCTION
    const soa_type& soa() const { return _soa; }

    //! Get the vector index of the particle in the SoA.
    KOKKOS_FORCEINLINE_FUNCTION
    int vectorIndex() const { return _vector_index; }

    //! The soa the particle is in.
    soa_type& _soa;

    //! The local vector index of the particle.
    int _vector_index;
};

//---------------------------------------------------------------------------//
// Particle accessors.
//---------------------------------------------------------------------------//
//! Return a single element of a single field from indices.
template <class FieldTag, class... FieldTags, class... IndexTypes>
KOKKOS_FORCEINLINE_FUNCTION typename std::enable_if<
    sizeof...( IndexTypes ) == FieldTag::rank,
    typename Particle<FieldTags...>::tuple_type::
        template member_const_reference_type<
            TypeIndexer<FieldTag, FieldTags...>::index>>::type
get( const Particle<FieldTags...>& particle, FieldTag, IndexTypes... indices )
{
    return Cabana::get<TypeIndexer<FieldTag, FieldTags...>::index>(
        particle.tuple(), indices... );
}

//! Return a single element of a single field from indices.
template <class FieldTag, class... FieldTags, class... IndexTypes>
KOKKOS_FORCEINLINE_FUNCTION typename std::enable_if<
    sizeof...( IndexTypes ) == FieldTag::rank,
    typename Particle<FieldTags...>::tuple_type::template member_reference_type<
        TypeIndexer<FieldTag, FieldTags...>::index>>::type
get( Particle<FieldTags...>& particle, FieldTag, IndexTypes... indices )
{
    return Cabana::get<TypeIndexer<FieldTag, FieldTags...>::index>(
        particle.tuple(), indices... );
}

//---------------------------------------------------------------------------//
//! Return a single element of a single field from indices.
template <class FieldTag, class... FieldTags, class... IndexTypes,
          int VectorLength>
KOKKOS_FORCEINLINE_FUNCTION typename std::enable_if<
    sizeof...( IndexTypes ) == FieldTag::rank,
    typename ParticleView<VectorLength, FieldTags...>::soa_type::
        template member_const_reference_type<
            TypeIndexer<FieldTag, FieldTags...>::index>>::type
get( const ParticleView<VectorLength, FieldTags...>& particle, FieldTag,
     IndexTypes... indices )
{
    return Cabana::get<TypeIndexer<FieldTag, FieldTags...>::index>(
        particle.soa(), particle.vectorIndex(), indices... );
}

//! Return a single element of a single field from indices.
template <class FieldTag, class... FieldTags, class... IndexTypes,
          int VectorLength>
KOKKOS_FORCEINLINE_FUNCTION typename std::enable_if<
    sizeof...( IndexTypes ) == FieldTag::rank,
    typename ParticleView<VectorLength, FieldTags...>::soa_type::
        template member_reference_type<
            TypeIndexer<FieldTag, FieldTags...>::index>>::type
get( ParticleView<VectorLength, FieldTags...>& particle, FieldTag,
     IndexTypes... indices )
{
    return Cabana::get<TypeIndexer<FieldTag, FieldTags...>::index>(
        particle.soa(), particle.vectorIndex(), indices... );
}

//---------------------------------------------------------------------------//
//! List of particle fields stored in AoSoA.
template <class MemorySpace, class... FieldTags>
class ParticleList
{
  public:
    //! Kokkos memory space.
    using memory_space = MemorySpace;
    //! Particle AoSoA member types.
    using traits = ParticleTraits<FieldTags...>;
    //! AoSoA type.
    using aosoa_type =
        Cabana::AoSoA<typename traits::member_types, memory_space>;
    //! Particle tuple type.
    using tuple_type = typename aosoa_type::tuple_type;
    /*!
      \brief Single field slice type.
      \tparam M AoSoA field index.
    */
    template <std::size_t M>
    using slice_type = typename aosoa_type::template member_slice_type<M>;
    //! Single particle type.
    using particle_type = Particle<FieldTags...>;
    //! Single SoA type.
    using particle_view_type =
        ParticleView<aosoa_type::vector_length, FieldTags...>;

    //! Default constructor.
    ParticleList( const std::string& label )
        : _aosoa( label )
        , _label( label )
    {
    }

    //! Get the number of particles in the list.
    std::size_t size() const { return _aosoa.size(); }

    //! Get the AoSoA.
    aosoa_type& aosoa() { return _aosoa; }
    //! Get the AoSoA (const).
    const aosoa_type& aosoa() const { return _aosoa; }

    //! Get the label.
    const std::string& label() const { return _label; }

    //! Get a slice of a given field.
    template <class FieldTag>
    slice_type<TypeIndexer<FieldTag, FieldTags...>::index>
    slice( FieldTag ) const
    {
        return Cabana::slice<TypeIndexer<FieldTag, FieldTags...>::index>(
            _aosoa, FieldTag::label() );
    }

  private:
    aosoa_type _aosoa;
    std::string _label;
};

//---------------------------------------------------------------------------//
//! ParticleList creation function.
template <class MemorySpace, class... FieldTags>
std::shared_ptr<ParticleList<MemorySpace, FieldTags...>>
createParticleList( const std::string& label, ParticleTraits<FieldTags...> )
{
    return std::make_shared<ParticleList<MemorySpace, FieldTags...>>( label );
}

//---------------------------------------------------------------------------//

} // end namespace Cabana

#endif // end CABANA_PARTICLELIST_HPP
