// Copyright (C) 2014 Oliver Schulz <oschulz@mpp.mpg.de>

// This is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.


#ifndef DBRX_TEXTBRICS_H
#define DBRX_TEXTBRICS_H

#include <iostream>
#include <fstream>

#include "Bric.h"


namespace dbrx {


class FromJSON: public TransformBric {
public:
	Input<std::string> input{this};
	Output<PropVal> output{this};

	void processInput() { output = PropVal::fromJSON(input); }

	using TransformBric::TransformBric;
};



class ToJSON: public TransformBric {
public:
	Input<PropVal> input{this};
	Output<std::string> output{this};

	void processInput() { output = input->toJSON(); }

	using TransformBric::TransformBric;
};



class TextFileReader: public MapperBric {
protected:
	std::istream* m_inputStream = nullptr;
	std::unique_ptr<std::ifstream> m_inputFile;

public:
	Input<std::string> input{this, "", "Input filename"};

	Output<std::string> output{this, "", "Output line"};

	void processInput();

	bool nextOutput();

	using MapperBric::MapperBric;
};



class TextFileWriter: public ReducerBric {
protected:
	std::ostream* m_outputStream = nullptr;
	std::unique_ptr<std::ofstream> m_outputFile;

public:
	Input<std::string> input{this, "", "Input line"};

	Param<std::string> target{this, "target", "Output filename", "stdout"};

	Output<ssize_t> output{this, "", "Number of lines in output"};

	void newReduction();

	void processInput();

	void finalizeReduction();

	using ReducerBric::ReducerBric;
};


} // namespace dbrx

#endif // DBRX_TEXTBRICS_H
