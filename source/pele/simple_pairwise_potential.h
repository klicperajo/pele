#ifndef PYGMIN_SIMPLE_PAIRWISE_POTENTIAL_H
#define PYGMIN_SIMPLE_PAIRWISE_POTENTIAL_H

#include <memory>
#include <vector>

#include "pairwise_potential_interface.h"
#include "array.h"
#include "vecn.h"
#include "distance.h"

namespace pele {

/**
 * Define a base class for potentials with simple pairwise interactions that
 * depend only on magnitude of the atom separation
 *
 * This class loops though atom pairs, computes the distances and get's the
 * value of the energy and gradient from the class pairwise_interaction.
 * pairwise_interaction is a passed parameter and defines the actual
 * potential function.
 */
template<typename pairwise_interaction,
         typename distance_policy = cartesian_distance<3>>
class SimplePairwisePotential : public PairwisePotentialInterface
{
protected:
    static const size_t m_ndim = distance_policy::_ndim;
    std::shared_ptr<pairwise_interaction> _interaction;
    std::shared_ptr<distance_policy> _dist;
    const double m_radii_sca;

    SimplePairwisePotential( std::shared_ptr<pairwise_interaction> interaction,
            const Array<double> radii,
            std::shared_ptr<distance_policy> dist=NULL,
            const double radii_sca=0.0)
        : PairwisePotentialInterface(radii),
          _interaction(interaction),
          _dist(dist),
          m_radii_sca(radii_sca)
    {
        if(_dist == NULL) _dist = std::make_shared<distance_policy>();
    }

    SimplePairwisePotential( std::shared_ptr<pairwise_interaction> interaction,
            std::shared_ptr<distance_policy> dist=NULL)
        : _interaction(interaction),
          _dist(dist),
          m_radii_sca(0.0)
    {
        if(_dist == NULL) _dist = std::make_shared<distance_policy>();
    }

public:
    virtual ~SimplePairwisePotential()
    {}
    virtual inline size_t get_ndim() const { return m_ndim; }

    virtual inline void get_rij(double * const r_ij, double const * const r1, double const * const r2) const
    {
        return _dist->get_rij(r_ij, r1, r2);
    }

    virtual double get_energy(Array<double> const & x);
    virtual double get_energy_gradient(Array<double> const & x, Array<double> & grad)
    {
        grad.assign(0);
        return add_energy_gradient(x, grad);
    }
    virtual double get_energy_gradient_hessian(Array<double> const & x, Array<double> & grad,
                                               Array<double> & hess)
    {
        grad.assign(0);
        hess.assign(0);
        return add_energy_gradient_hessian(x, grad, hess);
    }
    virtual double add_energy_gradient(Array<double> const & x, Array<double> & grad);
    virtual double add_energy_gradient_hessian(Array<double> const & x, Array<double> & grad,
                                               Array<double> & hess);
    virtual void get_neighbors(pele::Array<double> const & coords,
                                pele::Array<std::vector<size_t>> & neighbor_indss,
                                pele::Array<std::vector<std::vector<double>>> & neighbor_distss,
                                const double cutoff_factor = 1.0);
    virtual void get_neighbors_picky(pele::Array<double> const & coords,
                                      pele::Array<std::vector<size_t>> & neighbor_indss,
                                      pele::Array<std::vector<std::vector<double>>> & neighbor_distss,
                                      pele::Array<short> const & include_atoms,
                                      const double cutoff_factor = 1.0);
    virtual std::vector<size_t> get_overlaps(Array<double> const & coords);
    virtual inline double get_interaction_energy_gradient(double r2, double *gij, size_t atom_i, size_t atom_j) const
    {
        return _interaction->energy_gradient(r2, gij, sum_radii(atom_i, atom_j));
    }
    virtual inline double get_interaction_energy_gradient_hessian(double r2, double *gij, double *hij, size_t atom_i, size_t atom_j) const
    {
        return _interaction->energy_gradient_hessian(r2, gij, hij, sum_radii(atom_i, atom_j));
    }

    // Compute the maximum of all single atom norms
    virtual inline double compute_norm(pele::Array<double> const & x) {
        const size_t natoms = x.size() / m_ndim;
        double max_x = 0;
        if (m_soa)
        {
            #pragma simd reduction(max : max_x)
            for (size_t atom_i = 0;  atom_i < natoms; ++atom_i) {
                double atom_x = 0;
                #pragma unroll
                for (size_t j = 0; j < m_ndim; ++j) {
                    size_t ind = j * natoms + atom_i;
                    atom_x += x[ind] * x[ind];
                }
                max_x = std::max(max_x, atom_x);
            }
        } else {
            #pragma simd reduction(max : max_x)
            for (size_t atom_i = 0;  atom_i < natoms; ++atom_i) {
                double atom_x = 0;
                #pragma unroll
                for (size_t j = 0; j < m_ndim; ++j) {
                    size_t ind = atom_i * m_ndim + j;
                    atom_x += x[ind] * x[ind];
                }
                max_x = std::max(max_x, atom_x);
            }
        }
        return sqrt(max_x);
    }
};

template<typename pairwise_interaction, typename distance_policy>
inline double
SimplePairwisePotential<pairwise_interaction, distance_policy>::add_energy_gradient(
        Array<double> const & x, Array<double> & grad)
{
    const size_t natoms = x.size() / m_ndim;
    if (m_ndim * natoms != x.size()) {
        throw std::runtime_error("x.size() is not divisible by the number of dimensions");
    }
    if (grad.size() != x.size()) {
        throw std::runtime_error("grad must have the same size as x");
    }

    double e = 0.;
    double gij;
    double dr[m_ndim];

//    grad.assign(0.);

    for (size_t atom_i=0; atom_i<natoms; ++atom_i) {
        for (size_t atom_j=0; atom_j<atom_i; ++atom_j) {
            if (m_soa) {
                _dist->get_rij_soa(dr, &x[atom_i], &x[atom_j], natoms);
            } else {
                _dist->get_rij(dr, &x[atom_i*m_ndim], &x[atom_j*m_ndim]);
            }

            double r2 = 0;
            for (size_t k=0; k<m_ndim; ++k) {
                r2 += dr[k]*dr[k];
            }
            e += _interaction->energy_gradient(r2, &gij, sum_radii(atom_i, atom_j));
            if (gij != 0) {
                #pragma unroll
                for (size_t k=0; k<m_ndim; ++k) {
                    dr[k] *= gij;
                    if (m_soa) {
                        const size_t k1 = k * natoms;
                        grad[atom_i + k1] -= dr[k];
                        grad[atom_j + k1] += dr[k];
                    } else {
                        grad[atom_i*m_ndim + k] -= dr[k];
                        grad[atom_j*m_ndim + k] += dr[k];
                    }
                }
            }
        }
    }
    return e;
}

template<typename pairwise_interaction, typename distance_policy>
inline double SimplePairwisePotential<pairwise_interaction, distance_policy>::add_energy_gradient_hessian(
    Array<double> const & x, Array<double> & grad, Array<double> & hess)
{
    double hij, gij;
    double dr[m_ndim];
    const size_t N = x.size();
    const size_t natoms = x.size() / m_ndim;
    if (m_ndim * natoms != x.size()) {
        throw std::runtime_error("x.size() is not divisible by the number of dimensions");
    }
    if (x.size() != grad.size()) {
        throw std::invalid_argument("the gradient has the wrong size");
    }
    if (hess.size() != x.size() * x.size()) {
        throw std::invalid_argument("the Hessian has the wrong size");
    }
//    hess.assign(0.);
//    grad.assign(0.);

    double e = 0.;
    for (size_t atom_i=0; atom_i<natoms; ++atom_i) {
        for (size_t atom_j=0;atom_j<atom_i;++atom_j){

            if (m_soa) {
                _dist->get_rij_soa(dr, &x[atom_i], &x[atom_j], natoms);
            } else {
                _dist->get_rij(dr, &x[atom_i*m_ndim], &x[atom_j*m_ndim]);
            }
            double r2 = 0;
            for (size_t k=0;k<m_ndim;++k) {
                r2 += dr[k]*dr[k];
            }

            e += _interaction->energy_gradient_hessian(r2, &gij, &hij, sum_radii(atom_i, atom_j));

            if (gij != 0) {
                #pragma unroll
                for (size_t k=0; k<m_ndim; ++k) {
                    if (m_soa) {
                        const size_t k1 = k * natoms;
                        grad[atom_i + k1] -= gij * dr[k];
                        grad[atom_j + k1] += gij * dr[k];
                    } else {
                        grad[atom_i*m_ndim + k] -= gij * dr[k];
                        grad[atom_j*m_ndim + k] += gij * dr[k];
                    }
                }
            }

            if (hij != 0) {
                for (size_t k=0; k<m_ndim; ++k) {
                    if (m_soa) {
                        const size_t k1 = k * natoms;
                        //diagonal block - diagonal terms
                        double Hii_diag = (hij+gij)*dr[k]*dr[k]/r2 - gij;
                        hess[N*(atom_i + k1) + atom_i + k1] += Hii_diag;
                        hess[N*(atom_j + k1) + atom_j + k1] += Hii_diag;
                        //off diagonal block - diagonal terms
                        double Hij_diag = -Hii_diag;
                        hess[N*(atom_i + k1) + atom_j + k1] = Hij_diag;
                        hess[N*(atom_j + k1) + atom_i + k1] = Hij_diag;
                        for (size_t l = k+1; l<m_ndim; ++l){
                            const size_t l1 = l * natoms;
                            //diagonal block - off diagonal terms
                            double Hii_off = (hij+gij)*dr[k]*dr[l]/r2;
                            hess[N*(atom_i + k1) + atom_i + l1] += Hii_off;
                            hess[N*(atom_i + l1) + atom_i + k1] += Hii_off;
                            hess[N*(atom_j + k1) + atom_j + l1] += Hii_off;
                            hess[N*(atom_j + l1) + atom_j + k1] += Hii_off;
                            //off diagonal block - off diagonal terms
                            double Hij_off = -Hii_off;
                            hess[N*(atom_i + k1) + atom_j + l1] = Hij_off;
                            hess[N*(atom_i + l1) + atom_j + k1] = Hij_off;
                            hess[N*(atom_j + k1) + atom_i + l1] = Hij_off;
                            hess[N*(atom_j + l1) + atom_i + k1] = Hij_off;
                        }
                    } else {
                        const size_t i1 = atom_i * m_ndim;
                        const size_t j1 = atom_j * m_ndim;
                        //diagonal block - diagonal terms
                        double Hii_diag = (hij+gij)*dr[k]*dr[k]/r2 - gij;
                        hess[N*(i1 + k) + i1 + k] += Hii_diag;
                        hess[N*(j1 + k) + j1 + k] += Hii_diag;
                        //off diagonal block - diagonal terms
                        double Hij_diag = -Hii_diag;
                        hess[N*(i1 + k) + j1 + k] = Hij_diag;
                        hess[N*(j1 + k) + i1 + k] = Hij_diag;
                        for (size_t l = k+1; l<m_ndim; ++l) {
                            //diagonal block - off diagonal terms
                            double Hii_off = (hij+gij)*dr[k]*dr[l]/r2;
                            hess[N*(i1 + k) + i1 + l] += Hii_off;
                            hess[N*(i1 + l) + i1 + k] += Hii_off;
                            hess[N*(j1 + k) + j1 + l] += Hii_off;
                            hess[N*(j1 + l) + j1 + k] += Hii_off;
                            //off diagonal block - off diagonal terms
                            double Hij_off = -Hii_off;
                            hess[N*(i1 + k) + j1 + l] = Hij_off;
                            hess[N*(i1 + l) + j1 + k] = Hij_off;
                            hess[N*(j1 + k) + i1 + l] = Hij_off;
                            hess[N*(j1 + l) + i1 + k] = Hij_off;
                        }
                    }
                }
            }
        }
    }
    return e;
}

template<typename pairwise_interaction, typename distance_policy>
inline double SimplePairwisePotential<pairwise_interaction, distance_policy>::get_energy(
    Array<double> const & x)
{
    const size_t natoms = x.size() / m_ndim;
    if (m_ndim * natoms != x.size()) {
        throw std::runtime_error("x.size() is not divisible by the number of dimensions");
    }
    double e=0.;
    double dr[m_ndim];

    for (size_t atom_i=0; atom_i<natoms; ++atom_i) {
        for (size_t atom_j=0; atom_j<atom_i; ++atom_j) {
            if (m_soa) {
                _dist->get_rij_soa(dr, &x[atom_i], &x[atom_j], natoms);
            } else {
                _dist->get_rij(dr, &x[atom_i*m_ndim], &x[atom_j*m_ndim]);
            }
            double r2 = 0;
            for (size_t k=0;k<m_ndim;++k) {
                r2 += dr[k]*dr[k];
            }
            e += _interaction->energy(r2, sum_radii(atom_i, atom_j));
        }
    }
    return e;
}

template<typename pairwise_interaction, typename distance_policy>
void SimplePairwisePotential<pairwise_interaction, distance_policy>::get_neighbors(
    pele::Array<double> const & coords,
    pele::Array<std::vector<size_t>> & neighbor_indss,
    pele::Array<std::vector<std::vector<double>>> & neighbor_distss,
    const double cutoff_factor /*=1.0*/)
{
    const size_t natoms = coords.size() / m_ndim;
    pele::Array<short> include_atoms(natoms, 1);
    get_neighbors_picky(coords, neighbor_indss, neighbor_distss, include_atoms, cutoff_factor);
}

template<typename pairwise_interaction, typename distance_policy>
void SimplePairwisePotential<pairwise_interaction, distance_policy>::get_neighbors_picky(
    pele::Array<double> const & coords,
    pele::Array<std::vector<size_t>> & neighbor_indss,
    pele::Array<std::vector<std::vector<double>>> & neighbor_distss,
    pele::Array<short> const & include_atoms,
    const double cutoff_factor /*=1.0*/)
{
    const size_t natoms = coords.size() / m_ndim;
    if (m_ndim * natoms != coords.size()) {
        throw std::runtime_error("coords.size() is not divisible by the number of dimensions");
    }
    if (natoms != include_atoms.size()) {
        throw std::runtime_error("include_atoms.size() is not equal to the number of atoms");
    }
    if (m_radii.size() == 0) {
        throw std::runtime_error("Can't calculate neighbors, because the "
                                 "used interaction doesn't use radii. ");
    }
    std::vector<double> dr(m_ndim);
    std::vector<double> neg_dr(m_ndim);
    neighbor_indss = pele::Array<std::vector<size_t>>(natoms);
    neighbor_distss = pele::Array<std::vector<std::vector<double>>>(natoms);

    const double cutoff_sca = (1 + m_radii_sca) * cutoff_factor;

    for (size_t atom_i=0; atom_i<natoms; ++atom_i) {
        if (include_atoms[atom_i]) {
            for (size_t atom_j=0; atom_j<atom_i; ++atom_j) {
                if (include_atoms[atom_j]) {
                    _dist->get_rij(dr.data(), &coords[atom_i*m_ndim], &coords[atom_j*m_ndim]);
                    double r2 = 0;
                    for (size_t k=0;k<m_ndim;++k) {
                        r2 += dr[k]*dr[k];
                        neg_dr[k] = -dr[k];
                    }
                    const double r_H = sum_radii(atom_i, atom_j);
                    const double r_S = cutoff_sca * r_H;
                    const double r_S2 = r_S * r_S;
                    if(r2 <= r_S2) {
                        neighbor_indss[atom_i].push_back(atom_j);
                        neighbor_indss[atom_j].push_back(atom_i);
                        neighbor_distss[atom_i].push_back(dr);
                        neighbor_distss[atom_j].push_back(neg_dr);
                    }
                }
            }
        }
    }
}

template<typename pairwise_interaction, typename distance_policy>
std::vector<size_t> SimplePairwisePotential<pairwise_interaction, distance_policy>::get_overlaps(
    Array<double> const & coords)
{
    const size_t natoms = coords.size() / m_ndim;
    if (m_ndim * natoms != coords.size()) {
        throw std::runtime_error("coords.size() is not divisible by the number of dimensions");
    }
    if (m_radii.size() == 0) {
        throw std::runtime_error("Can't calculate neighbors, because the "
                                 "used interaction doesn't use radii. ");
    }
    pele::VecN<m_ndim, double> dr;
    std::vector<size_t> overlap_inds;

    for (size_t atom_i=0; atom_i<natoms; ++atom_i) {
        for (size_t atom_j=0; atom_j<atom_i; ++atom_j) {
            _dist->get_rij(dr.data(), &coords[atom_i*m_ndim], &coords[atom_j*m_ndim]);
            double r2 = 0;
            for (size_t k=0;k<m_ndim;++k) {
                r2 += dr[k]*dr[k];
            }
            const double r_H = sum_radii(atom_i, atom_j);
            const double r_H2 = r_H * r_H;
            if(r2 <= r_H2) {
                overlap_inds.push_back(atom_i);
                overlap_inds.push_back(atom_j);
            }
        }
    }
    return overlap_inds;
}

} // namespace pele

#endif // #ifndef PYGMIN_SIMPLE_PAIRWISE_POTENTIAL_H
