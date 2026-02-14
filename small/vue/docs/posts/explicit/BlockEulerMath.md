$$
\begin{aligned}
&\text{Input: } \mathbf{x}_n,\mathbf{v}_n,\mathbf{m},h\\
&\text{1: } \text{for all } i \in \mathcal{P} \text{ do}\\
&\text{2: }\quad \mathbf{f}_i \leftarrow \mathbf{f}^{spring}_i + \mathbf{f}^{gravity}_i\\
&\text{3: }\quad \mathbf{a}_i \leftarrow \mathbf{f}_i / m_i\\
&\text{4: }\quad \mathbf{v}_i^{n+1} \leftarrow \mathbf{v}_i^n + h\,\mathbf{a}_i\\
&\text{5: }\quad \mathbf{x}_i^{n+1} \leftarrow \mathbf{x}_i^n + h\,\mathbf{v}_i^{n+1}\\
&\text{6: } \text{end for}\\
&\text{Output: } \mathbf{x}_{n+1},\mathbf{v}_{n+1}
\end{aligned}
$$
