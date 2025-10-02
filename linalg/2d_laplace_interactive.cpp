// interactive sparse 2d laplace solver
// same problem as before, but (dirichlet) boundary conditions change with mouse input

#include <iostream>
#include <vector>

#include <Eigen/Sparse>
#include <raylib.h>

// typedef double Real;
typedef float Real;

using VectorXr = Eigen::Matrix<Real, Eigen::Dynamic, 1>;
using ArrayXr = Eigen::Array<Real, Eigen::Dynamic, 1>;
using MatrixXr = Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>;

typedef Eigen::SparseMatrix<Real>
        SpMat; // declares a column-major sparse matrix type of varying precision
typedef Eigen::Triplet<Real> T;

void build_matrix(std::vector<T> &coefficients, int n);
void build_rhs(VectorXr &b, VectorXr &boundary, int n);
VectorXr compute_boundary_condition(int n);
void render_to_texture(VectorXr &x, int n);
static Texture2D build_fill_texture(const SpMat &A, int texW, int texH);

// void assemble(); // fill up sparse mat
// void factor(); // run llt
// void rhs(); // set rhs
// void solve(); // solve for x
// void render();
// void visualize_coefficients();

int main() {
    int n = 300;
    int m = n * n;

    const int screen_width = n * 2;
    const int screen_height = n;

    std::vector<T> coefficients; // list of non-zeros coefficients
    VectorXr x(m);
    VectorXr b(m); // the right hand side-vector resulting from the constraints

    // Assembly:
    build_matrix(coefficients, n);

    SpMat A(m, m);
    A.setFromTriplets(coefficients.begin(), coefficients.end());
    // A \in SPD

    // Factor:
    Eigen::SimplicialCholesky<SpMat> solver;

    // Eigen::SparseLU<SpMat> solver;

    // Eigen::ConjugateGradient<SpMat, Eigen::Lower | Eigen::Upper, Eigen::IncompleteCholesky<Real>>
    //         solver;

    // Eigen::ConjugateGradient<SpMat, Eigen::Lower | Eigen::Upper, Eigen::IncompleteLUT<Real>>
    //         solver;
    // solver.preconditioner().setFillfactor(1);

    // Eigen::ConjugateGradient<SpMat, Eigen::Lower | Eigen::Upper,
    //                          Eigen::DiagonalPreconditioner<Real>>
    //         solver;

    // solver.setMaxIterations(200);
    // solver.setTolerance(1e-2);
    solver.compute(A);

    VectorXr condition = compute_boundary_condition(n);
    build_rhs(b, condition, n);
    x = solver.solve(b);

    InitWindow(screen_width, screen_height, __FILE__);

    // draw fill pattern once
    auto fill_texture = build_fill_texture(A, n, n);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // solve if there's mouse input
        if (GetMouseDelta().x != 0 || GetMouseDelta().y != 0) {
            // Condition
            VectorXr condition = compute_boundary_condition(n);
            build_rhs(b, condition, n);

            // Solve
            x = solver.solve(b);

            // std::cout << "iter: " << solver.iterations() << ", error: " << solver.error() << std::endl;
        }
        render_to_texture(x, n);

        // draw fill pattern texture
        DrawTexture(fill_texture, n, 0, WHITE);

        DrawFPS(10, 10);
        EndDrawing();
    }

    return 0;
}

VectorXr compute_boundary_condition(int n) {
    // t0: normalized mouse X in [0,1]
    const float sx = (float) n;
    const float mx = (float) GetMouseX();
    const float t0 = sx > 0.f ? std::max(0.0, std::min(1.0, (double) mx / (double) sx)) : 0.0;

    // t: [0,1] along x-axis (length n)
    ArrayXr t = ArrayXr::LinSpaced(n, 0.0, 1.0);

    ArrayXr g = (EIGEN_PI * (t - t0)).cos().square(); // in [0,1]

    return g.matrix(); // VectorXd
}

void set_coefficient(int id, int i, int j, double weight, std::vector<T> &coeffs, int n) {
    int id1 = i + j * n;
    if (!(i == -1 || i == n || j == -1 || j == n)) {
        coeffs.push_back(T(id, id1, weight));
    }
}

void set_rhs(int id, int i, int j, double weight, VectorXr &b, VectorXr &boundary, int n) {
    if (i == -1 || i == n)
        b(id) -= weight * boundary(j);
    else if (j == -1 || j == n)
        b(id) -= weight * boundary(i);
}

// called once
void build_matrix(std::vector<T> &coefficients, int n) {
    coefficients.clear();
    coefficients.reserve(n * n * 5);

    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            // describe linear relationship between the five cells
            int id = j * n + i;
            set_coefficient(id, i - 1, j, -1, coefficients, n);
            set_coefficient(id, i + 1, j, -1, coefficients, n);
            set_coefficient(id, i, j - 1, -1, coefficients, n);
            set_coefficient(id, i, j + 1, -1, coefficients, n);
            set_coefficient(id, i, j, 4, coefficients, n);
            // use positive weight on diagonals to make `A` positive definite
        }
    }
}

// called every frame
void build_rhs(VectorXr &b, VectorXr &condition, int n) {
    b.setZero();

    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            int id = j * n + i;
            // set boundary condition if cells are at the edge of the domain
            set_rhs(id, i - 1, j, -1, b, condition, n);
            set_rhs(id, i + 1, j, -1, b, condition, n);
            set_rhs(id, i, j - 1, -1, b, condition, n);
            set_rhs(id, i, j + 1, -1, b, condition, n);
            set_rhs(id, i, j, 4, b, condition, n);
        }
    }
}

void render_to_texture(VectorXr &x, int n) {
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

// Build a grayscale texture showing fill pattern of a sparse matrix (nonzero map)
// texW/texH: resolution of visualization (e.g., 1024x1024 or min(N, 1024))
static Texture2D build_fill_texture(const SpMat &A, int texW, int texH) {
    const int N = static_cast<int>(A.rows());
    if (A.cols() != N) {
        // return empty texture
        Image empty = GenImageColor(2, 2, BLACK);
        Texture2D t = LoadTextureFromImage(empty);
        UnloadImage(empty);
        return t;
    }

    texW = std::max(1, std::min(texW, N));
    texH = std::max(1, std::min(texH, N));

    const int rowBin = (N + texH - 1) / texH; // ceil(N/texH)
    const int colBin = (N + texW - 1) / texW;

    std::vector<unsigned char> bins(texW * texH, 0);

    // iterate nonzeros: column-major outer loop
    for (int c = 0; c < A.outerSize(); ++c) {
        for (SpMat::InnerIterator it(A, c); it; ++it) {
            int r = it.row();
            int px = c / colBin;
            int py = r / rowBin;
            int idx = py * texW + px;
            // accumulate density (saturate)
            unsigned int v = bins[idx] + 16; // adjust step for contrast
            bins[idx] = static_cast<unsigned char>(v > 255 ? 255 : v);
        }
    }

    // create grayscale texture and upload once
    Image img;
    img.data = bins.data();
    img.width = texW;
    img.height = texH;
    img.mipmaps = 1;
    img.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;

    Texture2D tex = LoadTextureFromImage(img);
    return tex;
}
