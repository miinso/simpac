$$
v_{n+1} = v_n + h a_n \\
v_{n+1} = v_n + h \frac{1}{m_i}f_i \\
\text{where} f_i is: \\
f_i = f_{spring} + f_{gravity} \\
x_{n+1} = x_n + v_{n+1}
$$


```c
struct Particle {
    vec3 Position;
}
```