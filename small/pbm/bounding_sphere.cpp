struct bounding_sphere {
    Eigen::Vector3f x;
    float r;

    bounding_sphere() : x(Eigen::Vector3f::Zero()), r(0.0f) {}
    bounding_sphere(const Eigen::Vector3f& x, float r) : x(x), r(r) {}
    bounding_sphere(const Eigen::Vector3f& a) : x(a), r(0.0f) {}
    
    bounding_sphere(const Eigen::Vector3f& a, const Eigen::Vector3f& b) {
        Eigen::Vector3f ba = b - a;
        x = (a + b) * 0.5f;
        r = 0.5f * ba.norm();
    }
    
    bounding_sphere(const Eigen::Vector3f& a, const Eigen::Vector3f& b, const Eigen::Vector3f& c) {
        Eigen::Vector3f ba = b - a;
        Eigen::Vector3f ca = c - a;
        Eigen::Vector3f baxca = ba.cross(ca);
        Eigen::Vector3f r;
        Eigen::Matrix3f T;
        T << ba[0], ba[1], ba[2],
             ca[0], ca[1], ca[2],
             baxca[0], baxca[1], baxca[2];

        r[0] = 0.5f * ba.squaredNorm();
        r[1] = 0.5f * ca.squaredNorm();
        r[2] = 0.0f;

        x = T.inverse() * r;
        r = x.norm();
        x += a;
    }
    
    bounding_sphere(const Eigen::Vector3f& a, const Eigen::Vector3f& b, 
                    const Eigen::Vector3f& c, const Eigen::Vector3f& d) {
        Eigen::Vector3f ba = b - a;
        Eigen::Vector3f ca = c - a;
        Eigen::Vector3f da = d - a;
        Eigen::Vector3f r;
        Eigen::Matrix3f T;
        T << ba[0], ba[1], ba[2],
             ca[0], ca[1], ca[2],
             da[0], da[1], da[2];

        r[0] = 0.5f * ba.squaredNorm();
        r[1] = 0.5f * ca.squaredNorm();
        r[2] = 0.5f * da.squaredNorm();
        x = T.inverse() * r;
        r = x.norm();
        x += a;
    }

    void set_points(const std::vector<Eigen::Vector3f>& p) {
        std::vector<Eigen::Vector3f> v(p);
        std::sort(v.begin(), v.end());
        v.erase(std::unique(v.begin(), v.end()), v.end());

        const float epsilon = 1.0e-6f;
        for (int i = v.size() - 1; i > 0; i--) {
            Eigen::Vector3f epsilon_vec = epsilon * Eigen::Vector3f::Random();
            int j = static_cast<int>(std::floor(i * static_cast<float>(rand()) / RAND_MAX));
            std::swap(v[i], v[j]);
            v[i] += epsilon_vec;
            v[j] -= epsilon_vec;
        }

        bounding_sphere S(v[0], v[1]);

        for (size_t i = 2; i < v.size(); i++) {
            Eigen::Vector3f d = v[i] - S.x;
            if (d.squaredNorm() > S.r * S.r) {
                S = ses1(i, v, v[i]);
            }
        }

        x = S.x;
        r = S.r + epsilon;
    }

    bool overlaps(const bounding_sphere& other) const {
        float rr = r + other.r;
        return (x - other.x).squaredNorm() < rr * rr;
    }

    bool contains(const bounding_sphere& other) const {
        float rr = r - other.r;
        return (x - other.x).squaredNorm() < rr * rr;
    }

    bool contains(const Eigen::Vector3f& point) const {
        return (x - point).squaredNorm() < r * r;
    }

private:
    bounding_sphere ses3(int n, std::vector<Eigen::Vector3f>& p, 
                         Eigen::Vector3f& q1, Eigen::Vector3f& q2, Eigen::Vector3f& q3) {
        bounding_sphere S(q1, q2, q3);

        for (int i = 0; i < n; i++) {
            Eigen::Vector3f d = p[i] - S.x;
            if (d.squaredNorm() > S.r * S.r) {
                S = bounding_sphere(q1, q2, q3, p[i]);
            }
        }
        return S;
    }

    bounding_sphere ses2(int n, std::vector<Eigen::Vector3f>& p, 
                         Eigen::Vector3f& q1, Eigen::Vector3f& q2) {
        bounding_sphere S(q1, q2);

        for (int i = 0; i < n; i++) {
            Eigen::Vector3f d = p[i] - S.x;
            if (d.squaredNorm() > S.r * S.r) {
                S = ses3(i, p, q1, q2, p[i]);
            }
        }
        return S;
    }

    bounding_sphere ses1(int n, std::vector<Eigen::Vector3f>& p, Eigen::Vector3f& q1) {
        bounding_sphere S(p[0], q1);

        for (int i = 1; i < n; i++) {
            Eigen::Vector3f d = p[i] - S.x;
            if (d.squaredNorm() > S.r * S.r) {
                S = ses2(i, p, q1, p[i]);
            }
        }
        return S;
    }
};