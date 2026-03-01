% same config as main.cpp's (just to validate the result)

P = [4 1; 1 2];
q = [1; 1];
A = [1 1; 1 0; 0 1];
lo = [1; 0; 0];
hi = [1; 0.7; 0.7];

x = quadprog(P, q, [], [], A(1,:), 1, lo(2:3), hi(2:3), [], optimoptions('quadprog','Display','off'));

fprintf('x = (%.4f, %.4f)\n', x);

% >> validate
% x = (0.3000, 0.7000)

% main.cpp:
% x = (0.3014, 0.6984)

% guess it's a match? it's off bc matlab's is not some admm variants??
