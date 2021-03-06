
namespace channel {

template<typename eT>
inline
Chan<eT>& identity(Chan<eT>& C) {
	if(!C.is_square()) throw std::runtime_error("not square");
	C.eye();
	return C;
}

template<typename eT>
inline
Chan<eT> identity(uint n) {
	Chan<eT> C(n, n);
	return identity(C);
}


template<typename eT>
inline
Chan<eT>& no_interference(Chan<eT>& C) {
	C.zeros();
	C.col(0).fill(eT(1));
	return C;
}

template<typename eT>
inline
Chan<eT> no_interference(uint n) {
	Chan<eT> C(n, 1);
	return no_interference(C);
}


template<typename eT>
inline
Chan<eT>& randu(Chan<eT>& C) {
	for(uint i = 0; i < C.n_rows; i++)
		C.row(i) = probab::randu<eT>(C.n_cols);

	return C;
}

template<typename eT>
inline
Chan<eT> randu(uint n) {
	Chan<eT> C(n, n);
	return randu(C);
}

template<typename eT>
inline
Chan<eT> randu(uint n, uint m) {
	Chan<eT> C(n, m);
	return randu(C);
}


template<typename eT>
inline
bool is_proper(const Chan<eT>& C, const eT& mrd = def_max_rel_diff<eT>()) {
	for(uint i = 0; i < C.n_rows; i++)
		if(!probab::is_proper<eT>(C.row(i), mrd))
			return false;

	return true;
}


template<typename eT>
inline
void check_proper(const Chan<eT>& C) {
	if(!is_proper(C))
		throw std::runtime_error("not a proper matrix");
}


template<typename eT>
inline
void check_prior_size(const Prob<eT>& pi, const Chan<eT>& C) {
	if(C.n_rows != pi.n_cols)
		throw std::runtime_error("invalid prior size");
}


template<typename eT>
inline bool equal(const Chan<eT>& A, const Chan<eT>& B, const eT& md = def_max_diff<eT>(), const eT& mrd = def_max_rel_diff<eT>()) {
	if(A.n_rows != B.n_rows || A.n_cols != B.n_cols)
		return false;

	for(uint i = 0; i < A.n_rows; i++)
		for(uint j = 0; j < A.n_cols; j++)
			if(!qif::equal(A.at(i, j), B.at(i, j), md, mrd))
				return false;

	return true;
}


// returns the posterior for a specific output y
//
template<typename eT>
inline
Prob<eT> posterior(const Chan<eT>& C, const Prob<eT>& pi, uint y) {
	return (arma::trans(C.col(y)) % pi) / arma::dot(C.col(y), pi);
}


// Returns a channel X such that A = B X
//
template<typename eT>
inline
Chan<eT> factorize_lp(const Chan<eT>& A, const Chan<eT>& B) {
	// A: M x N
	// B: M x R
	// X: R x N   unknowns
	//
	uint M = A.n_rows,
		 N = A.n_cols,
		 R = B.n_cols,
		 n_vars = R * N,			// one var for each element of X
		 n_cons = M * N + R,		// one constraint for each element of A (A[i,j] = dot(B[i,:], X[: j]), plus one constraint for row of X (sum = 1)
		 n_cons_elems = (M+1)*N*R;	// M*N*R elements for the A = B X constrains and N*R elements for the sum=1 constraints

	if(B.n_rows != M)
		return Chan<eT>();

	LinearProgram<eT> lp;
	lp.b.set_size(n_cons);
	lp.c = arma::zeros<Chan<eT>>(n_vars);			// we don't really care to optimize, so cost function = 0
	lp.sense.set_size(n_cons);
	lp.sense.fill('=');

	arma::umat locations(2, n_cons_elems);	// for batch-insertion into sparse matrix lp.A
	Col<eT> values(n_cons_elems);
	uint elem_i = 0;

	// Build equations for A = B X
	// We have R x N variables, that will be unfolded in a vector.
	// The varialbe X[r,n] will have variable number rN+n.
	// For each element m,n of A we have an equation A[i,j] = dot(B[i,:], X[: j])
	//
	for(uint m = 0; m < M; m++) {
		for(uint n = 0; n < N; n++) {
			uint row = m*N+n;
			lp.b(row) = A(m, n);				// sum of row is A[m,n]

			for(uint r = 0; r < R; r++) {
				locations(0, elem_i) = row;
				locations(1, elem_i) = r*N+n;
				values(elem_i) = B(m, r);		// coeff B[m,r] for variable X[r,n]
				elem_i++;
			}
		}
	}

	// equalities for summing up to 1
	//
	for(uint r = 0; r < R; r++) {
		uint row = M*N+r;
		lp.b(row) = eT(1);						// sum of row = 1

		for(uint n = 0; n < N; n++) {
			locations(0, elem_i) = row;
			locations(1, elem_i) = r*N+n;
			values(elem_i) = eT(1);				// coeff 1 for variable X[r,n]
			elem_i++;
		}
	}

	assert(elem_i == n_cons_elems);								// added all constraint elements

	lp.A = arma::SpMat<eT>(locations, values, n_cons, n_vars);	// arma has no batch-insert method into existing lp.A

	// solve program
	//
	if(!lp.solve())
		return Chan<eT>();

	// reconstrict channel from solution
	//
	Chan<eT> X(R, N);
	for(uint r = 0; r < R; r++)
		for(uint n = 0; n < N; n++)
			X(r, n) = lp.x(r*N+n);

	return X;
}

template<typename eT>
inline
Chan<eT>& project_to_simplex(Chan<eT>& C) {
	Prob<eT> temp(C.n_cols);
	for(uint i = 0; i < C.n_rows; i++) {
		temp = C.row(i);
		C.row(i) = probab::project_to_simplex(temp);
	}
	return C;
}


// factorize using a subgradient method.
// see: http://see.stanford.edu/materials/lsocoee364b/02-subgrad_method_notes.pdf
//
template<typename eT>
inline
Chan<eT> factorize_subgrad(const Chan<eT>& A, const Chan<eT>& B, const eT max_diff = 1e-4) {
	using arma::dot;

	const bool debug = false;

	// A: M x N
	// B: M x L
	// X: L x N   unknowns
	//
	uint M = A.n_rows,
		 N = A.n_cols,
		 L = B.n_cols;
	Chan<eT> X;

	if(B.n_rows != M)
		return X;

	// Solve B * X = A, if no solution exists then A is not factorizable
	//
	std::ostream nullstream(0);				// temporarily disable
	arma::set_stream_err2(nullstream);		// error messages
	arma::solve(X, B, A);
	arma::set_stream_err2(std::cout);

	if(!X.n_cols) return X;
	project_to_simplex(X);

	// G = max l2-norm of B's rows
	eT G(0);
	for(uint i = 0; i < M; i++)
		G = std::max(G, arma::norm(B.row(i), 2));

	const eT R = sqrt(2 * L);
		  	 //RG = R * G;
	const eT inf = infinity<eT>();

	Chan<eT> Z(M, N);
	Chan<eT> S = arma::zeros<Chan<eT>>(L, N);

	uint k;
	eT min(1), bound;
	eT sum1(- R * R), sum2(0);

	for(k = 1; true; k++) {
		// compute
		//    f = max_{i,j} | (B*X)(i,j) - A(i,j) |
		//      = max_{i,j,sign} sign*((B*X)(i,j) - A(i,j))     (sign in {1,-1})
		//
		// f_i, f_j, sign are the ones that give the max
		//
		Z = B * X - A;
		int sign = 1;
		eT f(0);
		uint f_i = 0, f_j = 0;

		for(uint i = 0; i < M; i++) {
			for(uint j = 0; j < N; j++) {
				eT diff = std::abs(Z(i, j));
				if(diff > f) {
					f = diff;
					f_i = i;
					f_j = j;
					sign = Z(i, j) < eT(0) ? -1 : 1;
				}
			}
		}

		// update min if we found a better one. when we reach zero we're done
		//
		if(f < min) {
			min = f;

			if(qif::equal(min, eT(0), max_diff))
				break;
		}

		// update X, using the CFM method in section 8 of the lecture notes
		//
		Col<eT> g = eT(sign) * B.row(f_i).t();	// the subgradient (this is actually the j-th col of the subgradient, all other cols are 0)
		eT g_norm_sq = dot(g, g);

		eT beta = std::max(eT(0), - eT(1.5) * dot(S.col(f_j), g) / g_norm_sq);
		S *= beta;
		S.col(f_j) += g;

		// S might have arbitrarily large values, such that its norm might become inf
		// In this case we reset it to just g (no memory). Note: this sounds safe but we should check
		eT s_norm_sq = arma::dot(S, S);
		if(s_norm_sq == inf) {
			S.fill(0);
			S.col(f_j) = g;
			s_norm_sq = g_norm_sq;
		}

		// alternative method with fixed beta (Note: for larger values of beta we were getting wrong results, maybe the bound does not hold for this method?)
		// const eT beta = 0.25;		// a high memory value seems to work well
		// S *= beta;
		// S.col(f_j) += (1-beta) * g;

		eT alpha = f / s_norm_sq;
		X -= alpha * S;

		project_to_simplex(X);

		// compute the lower bound given by the stopping criterion in section 3.4 of the lecture notes.
		// if the bound is positive then the optimal is also positive, so the channel cannot be factorized
		//
		sum1 += alpha * (2 * f - alpha * g_norm_sq);
		sum2 += 2 * alpha;
		bound = sum1 / sum2;

		if(!less_than_or_eq(bound, eT(0), max_diff)) {
			X.clear();
			break;
		}

		if(debug && k % 100 == 0)
			std::cout << "k: " << k << ", min: " << min << ", bound: " << bound << "\n";
	}

	if(debug)
		std::cout << "k: " << k << ", min: " << min << ", bound: " << bound << "\n";

	return X;
}

// Returns a channel X such that A = B X
//
template<typename eT>
inline
Chan<eT> factorize(const Chan<eT>& A, const Chan<eT>& B) {
	return factorize_subgrad(A, B);
}

template<>
inline
rchan factorize(const rchan& A, const rchan& B) {
	return factorize_lp(A, B);
}


template<typename eT>
inline void share_memory(Mat<eT>& A, Mat<eT>& B) {
	Mat<eT> temp(B.memptr(), B.n_rows, B.n_cols, false, false);

	A.reset();
	A.swap(temp);
}

} // namespace chan
