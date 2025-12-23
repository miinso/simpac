// sparse 2d laplace problem from https://libeigen.gitlab.io/eigen/docs-nightly/group__TutorialSparse.html
// sparse llt demo

#include <iostream>
#include <vector>

#include <Eigen/Sparse>
#include <raylib.h>

typedef Eigen::SparseMatrix<double> SpMat; // declares a column-major sparse matrix type of double
typedef Eigen::Triplet<double> T;

void build_problem(std::vector<T> &coefficients, Eigen::VectorXd &b, int n);
void save_as_bitmap(Eigen::VectorXd &x, int n);

int main() {
    int n = 300;
    int m = n * n;

    const int screen_width = n;
    const int screen_height = n;

    // Assembly:
    std::vector<T> coefficients; // list of non-zeros coefficients
    Eigen::VectorXd b(m); // the right hand side-vector resulting from the constraints
    build_problem(coefficients, b, n);

    SpMat A(m, m);
    A.setFromTriplets(coefficients.begin(), coefficients.end());

    // Solving:
    Eigen::SimplicialCholesky<SpMat> chol(A); // perform a Cholesky factorization of A
    // use the factorization to solve for the given right hand side
    Eigen::VectorXd x = chol.solve(b);

    std::cout << x.head(10) << std::endl;

    std::printf("Hello\n");

    InitWindow(screen_width, screen_height, __FILE__);

    // Export the result to a file:
    save_as_bitmap(x, n);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        save_as_bitmap(x, n);

        DrawFPS(10, 10);
        EndDrawing();
    }

    return 0;
}

void insert_coefficient(int id, int i, int j, double weight, std::vector<T> &coeffs,
                        Eigen::VectorXd &b, const Eigen::VectorXd &boundary) {
    int n = int(boundary.size());
    int id1 = i + j * n; // == size(x) == size(b)

    if (i == -1 || i == n)
        b(id) -= weight * boundary(j);
    else if (j == -1 || j == n)
        b(id) -= weight * boundary(i);
    else
        coeffs.push_back(T(id, id1, weight));
}

void build_problem(std::vector<T> &coefficients, Eigen::VectorXd &b, int n) {
    b.setZero();
    Eigen::ArrayXd boundary = Eigen::ArrayXd::LinSpaced(n, 0, EIGEN_PI).sin().pow(2);

    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            // describe linear relationship between the five cells
            int id = j * n + i;
            insert_coefficient(id, i - 1, j, -1, coefficients, b, boundary);
            insert_coefficient(id, i + 1, j, -1, coefficients, b, boundary);
            insert_coefficient(id, i, j - 1, -1, coefficients, b, boundary);
            insert_coefficient(id, i, j + 1, -1, coefficients, b, boundary);
            insert_coefficient(id, i, j, 4, coefficients, b, boundary);
        }
    }
}

// void save_as_bitmap(Eigen::VectorXd &x, int n) {
//     Eigen::Array<unsigned char, Eigen::Dynamic, Eigen::Dynamic> bits =
//             (x * 255).cast<unsigned char>();
//     // render stuff
//     for (int j = 0; j < n; ++j) {
//         for (int i = 0; i < n; ++i) {
//             DrawPixel(i, j, {bits.data()[i + j * n], 0, 0, 255});
//         }
//     }
// }

void save_as_bitmap(Eigen::VectorXd &x, int n) {
    static Texture2D texture = {0};

    if (texture.id == 0) {
        Image img;
        img.data = nullptr;
        img.width = n;
        img.height = n;
        img.mipmaps = 1;
        img.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
        texture = LoadTextureFromImage(img);
    }

    Eigen::Array<unsigned char, Eigen::Dynamic, Eigen::Dynamic> pixel_data =
            (x * 255).cast<unsigned char>();

    UpdateTexture(texture, pixel_data.data());
    DrawTexture(texture, 0, 0, WHITE);
}
