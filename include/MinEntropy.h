#ifndef _QIF_MinEntropy_h_
#define _QIF_MinEntropy_h_
/*
This file belongs to the LIBQIF library.
A Quantitative Information Flow C++ Toolkit Library.
Copyright (C) 2013  Universidad Nacional de Río Cuarto(National University of Río Cuarto).
Author: Martinelli Fernán - fmartinelli89@gmail.com - Universidad Nacional de Río Cuarto (Argentina)
LIBQIF Version: 1.0
Date: 12th Nov 2013
========================================================================
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

=========================================================================
*/
#include "LeakageMeasure.h"
#include "aux.h"

/*! \class MinEntropy
 *  \brief The Min-Entropy model of entropy.
 *
 *  For most information about the foundations of this theory see <a href="../papers/p1.pdf">here</a>
 */

template<typename eT>
class MinEntropy : public LeakageMeasure<eT> {
	public:
		using LeakageMeasure<eT>::LeakageMeasure;

		eT vulnerability(const Prob<eT>& pi);
		eT cond_vulnerability(const Prob<eT>& pi);
		eT max_mult_leakage();

		eT entropy(const Prob<eT>& pi)		{ return -qif::real_ops<eT>::log2(vulnerability(pi));		}
		eT cond_entropy(const Prob<eT>& pi)	{ return -qif::real_ops<eT>::log2(cond_vulnerability(pi));	}
		eT capacity()						{ return  qif::real_ops<eT>::log2(max_mult_leakage());		}

		virtual const char* class_name() {
			return "MinEntropy";
		}
};
#endif
