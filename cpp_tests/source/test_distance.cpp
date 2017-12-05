#include <random>
#include <numeric>
#include <iterator>

#include <gtest/gtest.h>

#include "pele/distance.h"

using pele::Array;
using pele::periodic_distance;
using pele::cartesian_distance;
using pele::leesedwards_distance;

static double const EPS = std::numeric_limits<double>::min();
#define EXPECT_NEAR_RELATIVE(A, B, T)  ASSERT_NEAR(A/(fabs(A)+fabs(B) + EPS), B/(fabs(A)+fabs(B) + EPS), T)

#define TEST_REPEAT 100
#define BOX_LENGTH 10.0

class DistanceTest :  public ::testing::Test {
public:

    pele::Array<double> x2[TEST_REPEAT];
    pele::Array<double> y2[TEST_REPEAT];
    pele::Array<double> x3[TEST_REPEAT];
    pele::Array<double> y3[TEST_REPEAT];
    pele::Array<double> x42[TEST_REPEAT];
    pele::Array<double> y42[TEST_REPEAT];

    virtual void SetUp()
    {
        std::mt19937_64 gen(42);
        std::uniform_real_distribution<double> dist(1, 2*BOX_LENGTH);

        for (size_t i_repeat = 0; i_repeat < TEST_REPEAT; i_repeat++)  {
            x2[i_repeat] = pele::Array<double>(2);
            y2[i_repeat] = pele::Array<double>(2);
            for (size_t j = 0; j < 2; ++j) {
                x2[i_repeat][j] = dist(gen);
                y2[i_repeat][j] = dist(gen);
            }
        }

        for (size_t i_repeat = 0; i_repeat < TEST_REPEAT; i_repeat++)  {
            x3[i_repeat] = pele::Array<double>(3);
            y3[i_repeat] = pele::Array<double>(3);
            for (size_t j = 0; j < 3; ++j) {
                x3[i_repeat][j] = dist(gen);
                y3[i_repeat][j] = dist(gen);
            }
        }

        for (size_t i_repeat = 0; i_repeat < TEST_REPEAT; i_repeat++)  {
            x42[i_repeat] = pele::Array<double>(42);
            y42[i_repeat] = pele::Array<double>(42);
            for (size_t j = 0; j < 42; ++j) {
                x42[i_repeat][j] = dist(gen);
                y42[i_repeat][j] = dist(gen);
            }
        }
    }
};


TEST_F(DistanceTest, CartesianDistanceNorm_Works)
{
    for(size_t i_repeat = 0; i_repeat < TEST_REPEAT; i_repeat++)  {
        // compute with pele
        double dx_p_2[2];
        double dx_p_3[3];
        double dx_p_42[42];
        cartesian_distance<2>().get_rij(dx_p_2, x2[i_repeat].data(), y2[i_repeat].data());
        cartesian_distance<3>().get_rij(dx_p_3, x3[i_repeat].data(), y3[i_repeat].data());
        cartesian_distance<42>().get_rij(dx_p_42, x42[i_repeat].data(), y42[i_repeat].data());
        double ds_p_2 = 0;
        double ds_p_3 = 0;
        double ds_p_42 = 0;
        // compute with std
        double dx2[2];
        double dx3[3];
        double dx42[42];
        for (size_t i = 0; i < 2; ++i) {
            dx2[i] = x2[i_repeat][i] - y2[i_repeat][i];
            ASSERT_DOUBLE_EQ(dx_p_2[i], dx2[i]);
            ds_p_2 += dx_p_2[i] * dx_p_2[i];
        }
        for (size_t i = 0; i < 3; ++i) {
            dx3[i] = x3[i_repeat][i] - y3[i_repeat][i];
            ASSERT_DOUBLE_EQ(dx_p_3[i], dx3[i]);
            ds_p_3 += dx_p_3[i] * dx_p_3[i];
        }
        for (size_t i = 0; i < 42; ++i) {
            dx42[i] = x42[i_repeat][i] - y42[i_repeat][i];
            ASSERT_DOUBLE_EQ(dx_p_42[i], dx42[i]);
            ds_p_42 += dx_p_42[i] * dx_p_42[i];
        }
        const double ds2 = std::inner_product(dx2, dx2 + 2, dx2, double(0));
        const double ds3 = std::inner_product(dx3, dx3 + 3, dx3, double(0));
        const double ds42 = std::inner_product(dx42, dx42 + 42, dx42, double(0));
        // compare norms
        ASSERT_DOUBLE_EQ(ds_p_2, ds2);
        ASSERT_DOUBLE_EQ(ds_p_3, ds3);
        ASSERT_DOUBLE_EQ(ds_p_42, ds42);
    }
}

TEST_F(DistanceTest, NearestImageConvention_Works)
{
    double x_out_of_box2[2];
    double x_boxed_true2[2];
    double x_boxed_per2[2];
    double x_out_of_box3[3];
    double x_boxed_true3[3];
    double x_boxed_per3[3];
    double x_out_of_box42[42];
    double x_boxed_per42[42];
    // The following means that "in box" is in [-8, 8].
    const double L = 16;
    x_out_of_box2[0] = -10;
    x_out_of_box2[1] = 20;
    x_boxed_true2[0] = 6;
    x_boxed_true2[1] = 4;
    std::copy(x_out_of_box2, x_out_of_box2 + 2, x_boxed_per2);
    pele::Array<double> xp2(x_boxed_per2, 2);
    periodic_distance<2>(pele::Array<double>(2, L)).put_in_box(xp2);
    for (size_t i = 0; i < 2; ++i) {
        EXPECT_DOUBLE_EQ(x_boxed_per2[i], x_boxed_true2[i]);
    }
    x_out_of_box3[0] = -9;
    x_out_of_box3[1] = 8.25;
    x_out_of_box3[2] = 12.12;
    x_boxed_true3[0] = 7;
    x_boxed_true3[1] = -7.75;
    x_boxed_true3[2] = -3.88;
    std::copy(x_out_of_box3, x_out_of_box3 + 3, x_boxed_per3);
    pele::Array<double> xp3(x_boxed_per3, 3);
    periodic_distance<3>(pele::Array<double>(3, L)).put_in_box(xp3);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_DOUBLE_EQ(x_boxed_per3[i], x_boxed_true3[i]);
    }
    // Assert that putting in box is irrelevant for distances.
    std::mt19937_64 gen(42);
    std::uniform_real_distribution<double> dist(-100, 100);
    for (size_t i = 0; i < 42; ++i) {
        x_out_of_box42[i] = dist(gen);
    }
    std::vector<double> ones(42, 1);
    double delta42[42];
    periodic_distance<42>(pele::Array<double>(42, L)).get_rij(delta42, &*ones.begin(), x_out_of_box42);
    const double d2_42_before = std::inner_product(delta42, delta42 + 42, delta42, double(0));
    std::copy(x_out_of_box42, x_out_of_box42 + 42, x_boxed_per42);
    pele::Array<double> xp42(x_boxed_per42, 42);
    periodic_distance<42>(pele::Array<double>(42, L)).put_in_box(xp42);
    for (size_t i = 0; i < 42; ++i) {
        EXPECT_LE(x_boxed_per42[i], 0.5 * L);
        EXPECT_LE(-0.5 * L, x_boxed_per42[i]);
    }
    periodic_distance<42>(pele::Array<double>(42, L)).get_rij(delta42, &*ones.begin(), x_boxed_per42);
    const double d2_42_after = std::inner_product(delta42, delta42 + 42, delta42, double(0));
    EXPECT_DOUBLE_EQ(d2_42_before, d2_42_after);
}

/** Check the periodic put_atom_in_box method at the box boundary
 */
TEST_F(DistanceTest, PeriodicPutAtomInBox_BoxBoundaryWorks)
{
    pele::Array<double> boxvec(2, 10);
    double boxboundary = boxvec[0] * 0.5;

    for (int i = -20; i <= 20; i++) {
        for (int j = -20; j <= 20; j++) {
            pele::Array<double> coords(2, 0);
            coords[0] = i * boxboundary + j * std::numeric_limits<double>::epsilon();
            periodic_distance<2>(boxvec).put_atom_in_box(coords.data());
            EXPECT_LE(coords[0], boxboundary);
            EXPECT_GE(coords[0], -boxboundary);
        }
    }
}

/** Check the Lees-Edwards put_atom_in_box method at the box boundary (corners)
 */
TEST_F(DistanceTest, LeesEdwardsPutAtomInBox_BoxBoundaryWorks)
{
    pele::Array<double> boxvec(2, 10);
    double shear = 0.1;
    double boxboundary = boxvec[0] * 0.5;

    for (int i_x = -21; i_x <= 21; i_x += 2) {
        for (int j_x = -20; j_x <= 20; j_x++) {
            for (int i_y = -21; i_y <= 21; i_y += 2) {
                for (int j_y = -20; j_y <= 20; j_y++) {
                    pele::Array<double> coords(2, 0);
                    double shear_corr;
                    if ((i_y < 0 && j_y > 0) || (i_y > 0 && j_y > 0)) {
                        shear_corr = std::ceil(i_y * 0.5) * shear * boxvec[1];
                    } else {
                        shear_corr = std::floor(i_y * 0.5) * shear * boxvec[1];
                    }
                    coords[0] = i_x * boxboundary
                                + j_x * std::numeric_limits<double>::epsilon()
                                + shear_corr;
                    coords[1] = i_y * boxboundary + j_y * std::numeric_limits<double>::epsilon();
                    leesedwards_distance<2>(boxvec, shear).put_atom_in_box(coords.data());
                    EXPECT_LE(coords[0], boxboundary);
                    EXPECT_GE(coords[0], -boxboundary);
                    EXPECT_LE(coords[1], boxboundary);
                    EXPECT_GE(coords[1], -boxboundary);
                }
            }
        }
    }
}

TEST_F(DistanceTest, SimplePeriodicNorm_Works)
{
    pele::Array<double> bv2(2, BOX_LENGTH);
    pele::Array<double> bv3(3, BOX_LENGTH);
    pele::Array<double> bv42(42, BOX_LENGTH);

    for(size_t i_repeat = 0; i_repeat < TEST_REPEAT; i_repeat++)  {

        // compute with periodic_distance
        double dx_periodic_2d[2];
        double dx_periodic_3d[3];
        double dx_periodic_42d[42];
        periodic_distance<2>(bv2).get_rij(dx_periodic_2d, x2[i_repeat].data(), y2[i_repeat].data());
        periodic_distance<3>(bv3).get_rij(dx_periodic_3d, x3[i_repeat].data(), y3[i_repeat].data());
        periodic_distance<42>(bv42).get_rij(dx_periodic_42d, x42[i_repeat].data(), y42[i_repeat].data());

        // compute directly and compare
        double dx;
        for (size_t i = 0; i < 2; ++i) {
            dx = x2[i_repeat][i] - y2[i_repeat][i];
            dx -= round(dx / BOX_LENGTH) * BOX_LENGTH;
            ASSERT_DOUBLE_EQ(dx_periodic_2d[i], dx);
        }
        for (size_t i = 0; i < 3; ++i) {
            dx = x3[i_repeat][i] - y3[i_repeat][i];
            dx -= round(dx / BOX_LENGTH) * BOX_LENGTH;
            ASSERT_DOUBLE_EQ(dx_periodic_3d[i], dx);
        }
        for (size_t i = 0; i < 42; ++i) {
            dx = x42[i_repeat][i] - y42[i_repeat][i];
            dx -= round(dx / BOX_LENGTH) * BOX_LENGTH;
            ASSERT_DOUBLE_EQ(dx_periodic_42d[i], dx);
        }
    }
}

/** Check the Lees Edward-image convention via the put_atom_in_box method using simple examples.
 */
TEST_F(DistanceTest, LeesEdwards_Image_Y)
{
    // Set up boxes
    pele::Array<double> bv2(2, 10);

    double shear = 0.1;

    // Set up test coordinates
    double r_test[][2] = {{3.7, 6.5}, {-2.2, 8.5}, {3.8, -7.7}, {-2.2, -6.9}, {4.8, -8.5}};
    double r_exp[][2] = {{2.7, -3.5}, {-3.2, -1.5}, {4.8, 2.3}, {-1.2, 3.1}, {-4.2, 1.5}};

    // Compute images using Lees-Edwards
    for(int i = 0; i < 4; i++) {

        leesedwards_distance<2>(bv2, shear).put_atom_in_box(r_test[i]);
        for(int j = 0; j < 2; j++) {
            ASSERT_DOUBLE_EQ(r_exp[i][j], r_test[i][j]);
        }

    }
}

/** Check the Lees Edward-image convention via the put_in_box method using simple examples.
 */
TEST_F(DistanceTest, LeesEdwards_Image_NotY)
{
    // Set up boxes
    pele::Array<double> bv2(2, BOX_LENGTH);
    pele::Array<double> bv3(3, BOX_LENGTH);
    pele::Array<double> bv42(42, BOX_LENGTH);

    for(size_t i_repeat = 0; i_repeat < TEST_REPEAT; i_repeat++)  {

        if(i_repeat > 20) {
            x2[0] *= 10;
            x3[0] *= 10;
            x42[0] *= 10;
        }
        if(i_repeat > 40) {
            x3[2] *= 10;
            x42[2] *= 10;
        }
        if(i_repeat > 60) {
            x42[5] *= 10;
        }
        if(i_repeat > 80) {
            x42[17] *= 10;
        }


        // Compute image with leesedwards_distance
        leesedwards_distance<2>(bv2, 0).put_atom_in_box(x2[i_repeat].data());
        leesedwards_distance<3>(bv3, 0).put_atom_in_box(x3[i_repeat].data());
        leesedwards_distance<42>(bv42, 0).put_atom_in_box(x42[i_repeat].data());

        // Compute image with periodic_distance
        pele::Array<double> x_periodic_2d = x2[i_repeat].copy();
        pele::Array<double> x_periodic_3d = x3[i_repeat].copy();
        pele::Array<double> x_periodic_42d = x42[i_repeat].copy();
        periodic_distance<2>(bv2).put_atom_in_box(x_periodic_2d.data());
        periodic_distance<3>(bv3).put_atom_in_box(x_periodic_3d.data());
        periodic_distance<42>(bv42).put_atom_in_box(x_periodic_42d.data());

        // Compare distances
        for(size_t i = 0; i < 2; i++) {
            ASSERT_DOUBLE_EQ(x_periodic_2d[i], x2[i_repeat].data()[i]);
        }
        for(size_t i = 0; i < 3; i++) {
            ASSERT_DOUBLE_EQ(x_periodic_3d[i], x3[i_repeat].data()[i]);
        }
        for(size_t i = 0; i < 42; i++) {
            ASSERT_DOUBLE_EQ(x_periodic_42d[i], x42[i_repeat].data()[i]);
        }
    }
}

/** At a shear of 0, the result should be
 *  the same as for periodic boundary conditions.
 */
TEST_F(DistanceTest, LeesEdwards_NoShear)
{
    // Set up boxes
    pele::Array<double> bv2(2, BOX_LENGTH);
    pele::Array<double> bv3(3, BOX_LENGTH);
    pele::Array<double> bv42(42, BOX_LENGTH);

    for(size_t i_repeat = 0; i_repeat < TEST_REPEAT; i_repeat++)  {

        // Compute distance with leesedwards_distance
        double dx_leesedwards_2d[2];
        double dx_leesedwards_3d[3];
        double dx_leesedwards_42d[42];
        leesedwards_distance<2>(bv2, 0).get_rij(dx_leesedwards_2d, x2[i_repeat].data(), y2[i_repeat].data());
        leesedwards_distance<3>(bv3, 0).get_rij(dx_leesedwards_3d, x3[i_repeat].data(), y3[i_repeat].data());
        leesedwards_distance<42>(bv42, 0).get_rij(dx_leesedwards_42d, x42[i_repeat].data(), y42[i_repeat].data());

        // Compute distance with periodic_distance
        double dx_periodic_2d[2];
        double dx_periodic_3d[3];
        double dx_periodic_42d[42];
        periodic_distance<2>(bv2).get_rij(dx_periodic_2d, x2[i_repeat].data(), y2[i_repeat].data());
        periodic_distance<3>(bv3).get_rij(dx_periodic_3d, x3[i_repeat].data(), y3[i_repeat].data());
        periodic_distance<42>(bv42).get_rij(dx_periodic_42d, x42[i_repeat].data(), y42[i_repeat].data());

        // Compare distances
        for(size_t i = 0; i < 2; i++) {
            ASSERT_DOUBLE_EQ(dx_periodic_2d[i], dx_leesedwards_2d[i]);
        }
        for(size_t i = 0; i < 3; i++) {
            ASSERT_DOUBLE_EQ(dx_periodic_3d[i], dx_leesedwards_3d[i]);
        }
        for(size_t i = 0; i < 42; i++) {
            ASSERT_DOUBLE_EQ(dx_periodic_42d[i], dx_leesedwards_42d[i]);
        }
    }
}

/** At a shear of multiples of 1, the result should be
 *  the same as for periodic boundary conditions.
 */
TEST_F(DistanceTest, LeesEdwards_ShearPeriodic)
{
    // Set up boxes
    pele::Array<double> bv2(2, BOX_LENGTH);
    pele::Array<double> bv3(3, BOX_LENGTH);
    pele::Array<double> bv42(42, BOX_LENGTH);

    for(int shear = 0; shear < 10; shear++) {
        for(size_t i_repeat = 0; i_repeat < TEST_REPEAT; i_repeat++)  {

            // Compute distance with leesedwards_distance
            double dx_leesedwards_2d[2];
            double dx_leesedwards_3d[3];
            double dx_leesedwards_42d[42];
            leesedwards_distance<2>(bv2, shear).get_rij(dx_leesedwards_2d, x2[i_repeat].data(), y2[i_repeat].data());
            leesedwards_distance<3>(bv3, shear).get_rij(dx_leesedwards_3d, x3[i_repeat].data(), y3[i_repeat].data());
            leesedwards_distance<42>(bv42, shear).get_rij(dx_leesedwards_42d, x42[i_repeat].data(), y42[i_repeat].data());

            // Compute distance with periodic_distance
            double dx_periodic_2d[2];
            double dx_periodic_3d[3];
            double dx_periodic_42d[42];
            periodic_distance<2>(bv2).get_rij(dx_periodic_2d, x2[i_repeat].data(), y2[i_repeat].data());
            periodic_distance<3>(bv3).get_rij(dx_periodic_3d, x3[i_repeat].data(), y3[i_repeat].data());
            periodic_distance<42>(bv42).get_rij(dx_periodic_42d, x42[i_repeat].data(), y42[i_repeat].data());

            /* Compare distances:
             * Can't assert to full precision, since we loose precision when adding dx,
             * which is a multiple of BOX_LENGTH and might therefore be far larger than
             * the distance in x-direction.
             */
            for(size_t i = 0; i < 2; i++) {
                ASSERT_NEAR(dx_periodic_2d[i], dx_leesedwards_2d[i], shear * 3e-15);
            }
            for(size_t i = 0; i < 3; i++) {
                ASSERT_NEAR(dx_periodic_3d[i], dx_leesedwards_3d[i], shear * 3e-15);
            }
            for(size_t i = 0; i < 42; i++) {
                ASSERT_NEAR(dx_periodic_42d[i], dx_leesedwards_42d[i], shear * 3e-15);
            }
        }
    }
}

/** Calculates the norm of a vector.
 */
template<typename dtype, size_t length>
dtype norm(const dtype (&vec)[length]) {
    dtype sum = 0;
    for(const dtype elem : vec) {
        sum += elem * elem;
    }
    return sum;
}

/** Check the Lees Edward-distances by comparison with distances computed
 *  using cartesian_distance, periodic_distance and direct calculations.
 */
TEST_F(DistanceTest, LeesEdwards_Shear)
{
    // Set up boxes
    pele::Array<double> bv2(2, BOX_LENGTH);
    pele::Array<double> bv3(3, BOX_LENGTH);
    pele::Array<double> bv42(42, BOX_LENGTH);

    for(double shear = 0.1; shear <= 1.0; shear += 0.1) {
        for(size_t i_repeat = 0; i_repeat < TEST_REPEAT; i_repeat++) {

            // Compute distance with leesedwards_distance
            double dx_leesedwards_2d[2];
            double dx_leesedwards_3d[3];
            double dx_leesedwards_42d[42];
            leesedwards_distance<2>(bv2, shear).get_rij(dx_leesedwards_2d, x2[i_repeat].data(), y2[i_repeat].data());
            leesedwards_distance<3>(bv3, shear).get_rij(dx_leesedwards_3d, x3[i_repeat].data(), y3[i_repeat].data());
            leesedwards_distance<42>(bv42, shear).get_rij(dx_leesedwards_42d, x42[i_repeat].data(), y42[i_repeat].data());

            // Compute distance with periodic_distance
            double dx_periodic_2d[2];
            double dx_periodic_3d[3];
            double dx_periodic_42d[42];
            periodic_distance<2>(bv2).get_rij(dx_periodic_2d, x2[i_repeat].data(), y2[i_repeat].data());
            periodic_distance<3>(bv3).get_rij(dx_periodic_3d, x3[i_repeat].data(), y3[i_repeat].data());
            periodic_distance<42>(bv42).get_rij(dx_periodic_42d, x42[i_repeat].data(), y42[i_repeat].data());

            // Compute distance with cartesian_distance
            double dx_cartesian_2d[2];
            double dx_cartesian_3d[3];
            double dx_cartesian_42d[42];
            cartesian_distance<2>().get_rij(dx_cartesian_2d, x2[i_repeat].data(), y2[i_repeat].data());
            cartesian_distance<3>().get_rij(dx_cartesian_3d, x3[i_repeat].data(), y3[i_repeat].data());
            cartesian_distance<42>().get_rij(dx_cartesian_42d, x42[i_repeat].data(), y42[i_repeat].data());

            // Get unshifted distance (Cartesian in y-direction)
            double dx_unshifted_2d[2] = {dx_periodic_2d[0], dx_cartesian_2d[1]};
            double dx_unshifted_3d[2] = {dx_periodic_3d[0], dx_cartesian_3d[1]};
            double dx_unshifted_42d[2] = {dx_periodic_42d[0], dx_cartesian_42d[1]};

            // Get distance using the shifted image in y-direction
            double dx_shifted_2d[2] =
                    {dx_cartesian_2d[0] - round(dx_cartesian_2d[1] / BOX_LENGTH) * shear * BOX_LENGTH,
                     dx_cartesian_2d[1] - round(dx_cartesian_2d[1] / BOX_LENGTH) * BOX_LENGTH};
            dx_shifted_2d[0] -= round(dx_shifted_2d[0] / BOX_LENGTH) * BOX_LENGTH;
            double dx_shifted_3d[2] =
                    {dx_cartesian_3d[0] - round(dx_cartesian_3d[1] / BOX_LENGTH) * shear * BOX_LENGTH,
                     dx_cartesian_3d[1] - round(dx_cartesian_3d[1] / BOX_LENGTH) * BOX_LENGTH};
            dx_shifted_3d[0] -= round(dx_shifted_3d[0] / BOX_LENGTH) * BOX_LENGTH;
            double dx_shifted_42d[2] =
                    {dx_cartesian_42d[0] - round(dx_cartesian_42d[1] / BOX_LENGTH) * shear * BOX_LENGTH,
                     dx_cartesian_42d[1] - round(dx_cartesian_42d[1] / BOX_LENGTH) * BOX_LENGTH};
            dx_shifted_42d[0] -= round(dx_shifted_42d[0] / BOX_LENGTH) * BOX_LENGTH;

            // Get minimum of shifted and unshifted distances
            double* dx_final_2d;
            if(norm(dx_shifted_2d) < norm(dx_unshifted_2d)) {
                dx_final_2d = dx_shifted_2d;
            } else {
                dx_final_2d = dx_unshifted_2d;
            }
            double* dx_final_3d;
            dx_final_3d = dx_periodic_3d;
            if(norm(dx_shifted_3d) < norm(dx_unshifted_3d)) {
                dx_final_3d[0] = dx_shifted_3d[0];
                dx_final_3d[1] = dx_shifted_3d[1];
            } else {
                dx_final_3d[0] = dx_unshifted_3d[0];
                dx_final_3d[1] = dx_unshifted_3d[1];
            }
            dx_final_3d[2] = dx_leesedwards_3d[2];
            double* dx_final_42d;
            dx_final_42d = dx_periodic_42d;
            if(norm(dx_shifted_42d) < norm(dx_unshifted_42d)) {
                dx_final_42d[0] = dx_shifted_42d[0];
                dx_final_42d[1] = dx_shifted_42d[1];
            } else {
                dx_final_42d[0] = dx_unshifted_42d[0];
                dx_final_42d[1] = dx_unshifted_42d[1];
            }

            // Compare distances
            for(size_t i = 0; i < 2; i++) {
                ASSERT_DOUBLE_EQ(dx_final_2d[i], dx_leesedwards_2d[i]);
            }
            for(size_t i = 0; i < 3; i++) {
                ASSERT_DOUBLE_EQ(dx_final_3d[i], dx_leesedwards_3d[i]);
            }
            for(size_t i = 0; i < 42; i++) {
                ASSERT_DOUBLE_EQ(dx_final_42d[i], dx_leesedwards_42d[i]);
            }
        }
    }
}


void aos_to_soa(pele::Array<double> const & coords_aos, pele::Array<double>& coords_soa,
                const size_t natoms, const size_t ndim) {
    for (size_t idim = 0; idim < ndim; ++idim) {
        for (size_t iatom = 0; iatom < natoms; ++iatom) {
            coords_soa[idim*natoms + iatom] = coords_aos[iatom*ndim + idim];
        }
    }
}

TEST_F(DistanceTest, SoA_cartesian_rij)
{
    size_t seed = 42;
    std::mt19937_64 generator = std::mt19937_64(seed);
    std::uniform_real_distribution<double> distribution;
    distribution = std::uniform_real_distribution<double>(-1, 1);

    // Set up test coordinates
    size_t natoms = 100;
    size_t ndim = 3;
    size_t ndof = natoms * ndim;
    Array<double> x_aos = Array<double>(ndof);
    Array<double> x_soa = Array<double>(ndof);
    for (size_t i = 0; i < natoms; i++) {
        x_aos[i*ndim] = distribution(generator);
        x_aos[i*ndim + 1] = distribution(generator);
        x_aos[i*ndim + 2] = distribution(generator);
    }
    aos_to_soa(x_aos, x_soa, natoms, ndim);

    // Compute distances using Lees-Edwards
    Array<double> rij_aos(ndim);
    Array<double> rij_soa(ndim);
    for(int i = 0; i < 10; i++) {
        cartesian_distance<3>().get_rij(rij_aos.data(), x_aos.data() + i*ndim, x_aos.data() + ((i+1)%10)*ndim);
        cartesian_distance<3>().get_rij_soa(rij_soa.data(), x_soa.data() + i, x_soa.data() + (i+1)%10, natoms);
        for(int j = 0; j < ndim; j++) {
            ASSERT_DOUBLE_EQ(rij_aos[j], rij_soa[j]);
        }
    }
}
TEST_F(DistanceTest, SoA_periodic_rij)
{
    size_t seed = 42;
    std::mt19937_64 generator = std::mt19937_64(seed);
    std::uniform_real_distribution<double> distribution;
    distribution = std::uniform_real_distribution<double>(-1, 1);

    // Set up test coordinates
    size_t natoms = 100;
    size_t ndim = 3;
    size_t ndof = natoms * ndim;
    Array<double> x_aos = Array<double>(ndof);
    Array<double> x_soa = Array<double>(ndof);
    for (size_t i = 0; i < natoms; i++) {
        x_aos[i*ndim] = distribution(generator);
        x_aos[i*ndim + 1] = distribution(generator);
        x_aos[i*ndim + 2] = distribution(generator);
    }
    aos_to_soa(x_aos, x_soa, natoms, ndim);

    // Set up boxes
    pele::Array<double> bv3(3, 1.0);

    // Compute distances using Lees-Edwards
    Array<double> rij_aos(ndim);
    Array<double> rij_soa(ndim);
    for(int i = 0; i < 10; i++) {
        periodic_distance<3>(bv3).get_rij(rij_aos.data(), x_aos.data() + i*ndim, x_aos.data() + ((i+1)%10)*ndim);
        periodic_distance<3>(bv3).get_rij_soa(rij_soa.data(), x_soa.data() + i, x_soa.data() + (i+1)%10, natoms);
        for(int j = 0; j < ndim; j++) {
            ASSERT_DOUBLE_EQ(rij_aos[j], rij_soa[j]);
        }
    }
}
TEST_F(DistanceTest, SoA_LeesEdwards_rij)
{
    size_t seed = 42;
    std::mt19937_64 generator = std::mt19937_64(seed);
    std::uniform_real_distribution<double> distribution;
    distribution = std::uniform_real_distribution<double>(-1, 1);

    // Set up test coordinates
    size_t natoms = 100;
    size_t ndim = 3;
    size_t ndof = natoms * ndim;
    Array<double> x_aos = Array<double>(ndof);
    Array<double> x_soa = Array<double>(ndof);
    for (size_t i = 0; i < natoms; i++) {
        x_aos[i*ndim] = distribution(generator);
        x_aos[i*ndim + 1] = distribution(generator);
        x_aos[i*ndim + 2] = distribution(generator);
    }
    aos_to_soa(x_aos, x_soa, natoms, ndim);

    // Set up boxes
    pele::Array<double> bv3(3, 1.0);
    double shear = 0.1;

    // Compute distances using Lees-Edwards
    Array<double> rij_aos(ndim);
    Array<double> rij_soa(ndim);
    for(int i = 0; i < 10; i++) {
        leesedwards_distance<3>(bv3, shear).get_rij(rij_aos.data(), x_aos.data() + i*ndim, x_aos.data() + ((i+1)%10)*ndim);
        leesedwards_distance<3>(bv3, shear).get_rij_soa(rij_soa.data(), x_soa.data() + i, x_soa.data() + (i+1)%10, natoms);
        for(int j = 0; j < ndim; j++) {
            ASSERT_DOUBLE_EQ(rij_aos[j], rij_soa[j]);
        }
    }
}

TEST_F(DistanceTest, SoA_periodic_image)
{
    size_t seed = 42;
    std::mt19937_64 generator = std::mt19937_64(seed);
    std::uniform_real_distribution<double> distribution;
    distribution = std::uniform_real_distribution<double>(-1, 1);

    // Set up test coordinates
    size_t natoms = 100;
    size_t ndim = 3;
    size_t ndof = natoms * ndim;
    Array<double> x_aos = Array<double>(ndof);
    Array<double> x_soa = Array<double>(ndof);
    for (size_t i = 0; i < natoms; i++) {
        x_aos[i*ndim] = distribution(generator);
        x_aos[i*ndim + 1] = distribution(generator);
        x_aos[i*ndim + 2] = distribution(generator);
    }
    aos_to_soa(x_aos, x_soa, natoms, ndim);

    // Set up boxes
    pele::Array<double> bv3(3, 0.5);

    // Compute distances using Lees-Edwards
    Array<double> xbox_aos(ndim);
    Array<double> xbox_soa(ndim);
    for(int i = 0; i < 10; i++) {
        periodic_distance<3>(bv3).put_atom_in_box(xbox_aos.data(), x_aos.data() + i*ndim);
        periodic_distance<3>(bv3).put_atom_in_box_soa(xbox_soa.data(), x_soa.data() + i, natoms);
        periodic_distance<3>(bv3).put_atom_in_box(x_aos.data() + i*ndim);
        periodic_distance<3>(bv3).put_atom_in_box_soa(x_soa.data() + i, natoms);
        for(int j = 0; j < ndim; j++) {
            ASSERT_DOUBLE_EQ(xbox_aos[j], xbox_soa[j]);
            ASSERT_DOUBLE_EQ(xbox_soa[j], x_soa[i + j*natoms]);
            ASSERT_DOUBLE_EQ(x_aos[i*ndim + j], x_soa[i + j*natoms]);
        }
    }
}
TEST_F(DistanceTest, SoA_LeesEdwards_image)
{
    size_t seed = 42;
    std::mt19937_64 generator = std::mt19937_64(seed);
    std::uniform_real_distribution<double> distribution;
    distribution = std::uniform_real_distribution<double>(-1, 1);

    // Set up test coordinates
    size_t natoms = 100;
    size_t ndim = 3;
    size_t ndof = natoms * ndim;
    Array<double> x_aos = Array<double>(ndof);
    Array<double> x_soa = Array<double>(ndof);
    for (size_t i = 0; i < natoms; i++) {
        x_aos[i*ndim] = distribution(generator);
        x_aos[i*ndim + 1] = distribution(generator);
        x_aos[i*ndim + 2] = distribution(generator);
    }
    aos_to_soa(x_aos, x_soa, natoms, ndim);

    // Set up boxes
    pele::Array<double> bv3(3, 0.5);
    double shear = 0.1;

    // Compute distances using Lees-Edwards
    Array<double> xbox_aos(ndim);
    Array<double> xbox_soa(ndim);
    for(int i = 0; i < 10; i++) {
        leesedwards_distance<3>(bv3, shear).put_atom_in_box(xbox_aos.data(), x_aos.data() + i*ndim);
        leesedwards_distance<3>(bv3, shear).put_atom_in_box_soa(xbox_soa.data(), x_soa.data() + i, natoms);
        leesedwards_distance<3>(bv3, shear).put_atom_in_box(x_aos.data() + i*ndim);
        leesedwards_distance<3>(bv3, shear).put_atom_in_box_soa(x_soa.data() + i, natoms);
        for(int j = 0; j < ndim; j++) {
            ASSERT_DOUBLE_EQ(xbox_aos[j], xbox_soa[j]);
            ASSERT_DOUBLE_EQ(xbox_soa[j], x_soa[i + j*natoms]);
            ASSERT_DOUBLE_EQ(x_aos[i*ndim + j], x_soa[i + j*natoms]);
        }
    }
}
