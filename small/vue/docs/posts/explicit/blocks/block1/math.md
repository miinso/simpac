$$
\begin{aligned}
x_{n+1} &= x_n + h\,v_n \\
v_{n+1} &= v_n + h\,m^{-1}\left(\t{b1_spring}{f_{spring}} + \t{b1_gravity}{f_{gravity}}\right)
\end{aligned}
$$

> [!TIP]
> Optional information to help a user be more successful.

```c
struct Particle {
    vec3 x;
    vec3 v;
    vec3 f;
}
```

```js
export default {
  data () {
    return {
      msg: 'Removed' // [!code --]
      msg: 'Added' // [!code ++]
    }
  }
}
```