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


#ifndef DBRX_BRIC_H
#define DBRX_BRIC_H

#include <memory>
#include <atomic>
#include <stdexcept>
#include <map>
#include <iosfwd>

#include "Name.h"
#include "HasValue.h"
#include "logging.h"


namespace dbrx {


class Bric;
class BricWithOutputs;
class BricWithInputs;



class BricComponent: public virtual HasName, public virtual Configurable {
protected:
	struct NotReconfigurable : public std::runtime_error { using std::runtime_error::runtime_error; };

	virtual void connectInputToInner(Bric &bric, Name inputName, PropPath::Fragment sourcePath) = 0;

	virtual void setParent(Bric *parentBric) = 0;

public:
	PropPath absolutePath() const;

	virtual bool hasParent() const = 0;

	virtual const Bric& parent() const = 0;
	virtual Bric& parent() = 0;

	bool isInside(const Bric& other) const;

	BricComponent& operator=(const BricComponent& v) = delete;
	BricComponent& operator=(BricComponent &&v) = delete;

	friend class Bric;
};


class BricComponentImpl: public virtual BricComponent {
protected:
	Name m_name;
	Bric *m_parent = nullptr;

	virtual void setParent(Bric *parentBric) { m_parent = parentBric; }

public:
	Name name() const { return m_name; }
	Name& name() { return m_name; }

	bool hasParent() const { return m_parent != nullptr; }

	const Bric& parent() const { return *m_parent; }
	Bric& parent() { return *m_parent; }

	virtual bool syncedInput() const { return false; }

	BricComponentImpl() {}
	BricComponentImpl(Name componentName): m_name(componentName) {}
	BricComponentImpl(Bric *parentBric, Name componentName);
};



class Bric: public virtual BricComponent {
public:
	class Terminal: public virtual BricComponent, public virtual HasValue {
	protected:
		virtual void connectInputToInner(Bric &bric, Name inputName, PropPath::Fragment sourcePath);
	};


	class OutputTerminal: public virtual Terminal, public virtual HasWritableValue {};


	class InputTerminal: public virtual Terminal, public virtual HasConstValueRef {
	public:
		virtual const PropPath& source() = 0;

		virtual void connectTo(const Terminal &other) = 0;

		virtual void connectTo(const Bric &bric, Name terminalName) = 0;

		virtual void connectTo(const Bric &bric) = 0;
	};


	class ParamTerminal : public virtual Terminal, public virtual HasWritableValue {};

protected:
	static const Name s_defaultInputName;
	static const Name s_defaultOutputName;
	static const Name s_bricTypeKey;

	std::map<Name, BricComponent*> m_components;
	std::map<Name, Bric*> m_brics;
	std::map<Name, Terminal*> m_terminals;
	std::map<Name, ParamTerminal*> m_params;
	std::map<Name, OutputTerminal*> m_outputs;
	std::map<Name, InputTerminal*> m_inputs;

	std::map<Name, std::unique_ptr<Bric>> m_dynBrics;

	void registerComponent(BricComponent* component);
	void registerBric(Bric* bric);
	void registerTerminal(Terminal* terminal);
	void registerParam(ParamTerminal* param);
	void registerOutput(OutputTerminal* output);
	void registerInput(InputTerminal* input);

	bool isBricConfig(const PropVal& config);

	virtual void addDynBric(Name bricName, const PropVal& config);
	virtual void delDynBric(Name bricName);

	virtual void connectInputToInner(Bric &bric, Name inputName, PropPath::Fragment sourcePath);
	virtual void connectInputToSiblingOrUp(Bric &bric, Name inputName, PropPath::Fragment sourcePath);
	virtual void connectOwnInputTo(Name inputName, const Terminal& terminal);

public:
	template <typename T> class TypedTerminal
		: public virtual Terminal, public virtual HasTypedValue<T> {};


	template <typename T> class TypedOutputTerminal
		: public virtual TypedTerminal<T>, public virtual OutputTerminal, public virtual HasTypedWritableValue<T>
	{
	public:
		virtual void applyConfig(const PropVal& config) { if (!config.isNone()) throw std::invalid_argument("Output is not configurable"); }
		virtual PropVal getConfig() const  { return PropVal(); }

		TypedOutputTerminal<T>& operator=(const T &v)
			{ HasTypedWritableValue<T>::operator=(v); return *this; }

		TypedOutputTerminal<T>& operator=(T &&v) noexcept
			{ HasTypedWritableValue<T>::operator=(std::move(v)); return *this; }

		TypedOutputTerminal<T>& operator=(std::unique_ptr<T> &&v) noexcept
			{ HasTypedWritableValue<T>::operator=(std::move(v)); return *this; }
	};


	template <typename T> class TypedInputTerminal
		: public virtual TypedTerminal<T>, public virtual InputTerminal, public virtual HasTypedConstValueRef<T> {};


	template <typename T> class TypedParamTerminal
		: public virtual TypedTerminal<T>, public virtual ParamTerminal, public virtual HasTypedWritableValue<T>
	{
	public:
		TypedParamTerminal<T>& operator=(const T &v)
			{ HasTypedWritableValue<T>::operator=(v); return *this; }

		TypedParamTerminal<T>& operator=(T &&v) noexcept
			{ HasTypedWritableValue<T>::operator=(std::move(v)); return *this; }

		TypedParamTerminal<T>& operator=(std::unique_ptr<T> &&v) noexcept
			{ HasTypedWritableValue<T>::operator=(std::move(v)); return *this; }
	};


	template <typename T> class Param
		: public virtual TypedParamTerminal<T>, public BricComponentImpl, public HasTypedPrimaryValueImpl<T>
	{
	public:
		using HasTypedPrimaryValueImpl<T>::value;
		using HasTypedPrimaryValueImpl<T>::typeInfo;

		virtual void applyConfig(const PropVal& config) {
			PropVal currCfg = getConfig();
			if (currCfg.isProps() && config.isProps())
				assign_from(value().get(), currCfg.asProps() + config.asProps());
			else
				assign_from(value().get(), config);
		}

		virtual PropVal getConfig() const { PropVal config; assign_from(config, value().get()); return config; }

		Param<T>& operator=(const Param<T>& v) = delete;

		Param<T>& operator=(const T &v)
			{ TypedParamTerminal<T>::operator=(v); return *this; }

		Param<T>& operator=(T &&v) noexcept
			{ TypedParamTerminal<T>::operator=(std::move(v)); return *this; }

		Param<T>& operator=(std::unique_ptr<T> &&v) noexcept
			{ TypedParamTerminal<T>::operator=(std::move(v)); return *this; }


		Param(Bric *parentBric, Name paramName)
			: BricComponentImpl(parentBric, paramName) { parentBric->registerParam(this); }

		Param(const Param &other) = delete;
	};

	virtual bool hasSingleValueOutput() const { return false; }

	virtual bool outputSyncedWith(const Bric& other) {
		return hasSingleValueOutput() && other.hasSingleValueOutput()
			&& isInside(other);
	}

	virtual void applyConfig(const PropVal& config);
	virtual PropVal getConfig() const;

	virtual bool canHaveInputs() const { return false; }
	virtual bool canHaveOutputs() const { return false; }

	virtual bool canHaveDynBrics() { return false; }

	virtual const Terminal& getTerminal(Name terminalName) const;
	virtual Terminal& getTerminal(Name terminalName);

	virtual const Terminal& getTerminal(Name terminalName, const std::type_info& typeInfo) const;
	virtual Terminal& getTerminal(Name terminalName, const std::type_info& typeInfo);


	virtual const OutputTerminal& getOutput(Name outputName) const;
	virtual OutputTerminal& getOutput(Name outputName);

	virtual const OutputTerminal& getOutput(Name outputName, const std::type_info& typeInfo) const;
	virtual OutputTerminal& getOutput(Name outputName, const std::type_info& typeInfo);


	virtual const InputTerminal& getInput(Name outputName) const;
	virtual InputTerminal& getInput(Name outputName);

	virtual const InputTerminal& getInput(Name outputName, const std::type_info& typeInfo) const;
	virtual InputTerminal& getInput(Name outputName, const std::type_info& typeInfo);


	virtual const ParamTerminal& getParam(Name outputName) const;
	virtual ParamTerminal& getParam(Name outputName);

	virtual const ParamTerminal& getParam(Name outputName, const std::type_info& typeInfo) const;
	virtual ParamTerminal& getParam(Name outputName, const std::type_info& typeInfo);

	void connectInputs();

	virtual std::ostream & printInfo(std::ostream &os) const;

	// Undefined when run in relation to sub-bric init (possibly in parallel?):
	virtual void init() {};

	// Run before init_parentFirst for sub-brics:
	virtual void init_parentFirst() {};

	// Run after init_childrenFirst for sub-brics:
	virtual void init_childrenFirst() {};

protected:
	bool m_execFinished = false;

	// See nextExecStep for guarantees on behaviour and return value.
	virtual bool nextExecStepImpl() = 0;

	void setOutputsToErrorState() {
		dbrx_log_info("Due to an error, setting outputs of bric \"%s\" to default values", absolutePath());
		for (auto& output: m_outputs) output.second->value().setToDefault();
	}


// Sources //

protected:
	std::vector<Bric*> m_sources;

	std::atomic<size_t> m_nSourcesAvailable;

	std::atomic<size_t> m_nSourcesFinished;

	void addSource(Bric* source) { m_sources.push_back(source); }


	void incNSourcesAvailable() { atomic_fetch_add(&m_nSourcesAvailable, size_t(1)); }
	void decNSourcesAvailable() { atomic_fetch_sub(&m_nSourcesAvailable, size_t(1)); }
	void clearNSourcesAvailable() { atomic_store(&m_nSourcesAvailable, size_t(0)); }

	size_t nSourcesAvailable() const {
		size_t nAvail = atomic_load(&m_nSourcesAvailable);
		assert(nAvail <= m_sources.size()); // Sanity check
		if (nAvail > 0) assert(!allSourcesFinished()); // Sanity check
		return nAvail;
	}

	bool allSourcesAvailable() const { return nSourcesAvailable() == m_sources.size(); }
	bool anySourceAvailable() const { return nSourcesAvailable() > 0; }


	void incSourcesFinished() { atomic_fetch_add(&m_nSourcesFinished, size_t(1)); }

	size_t nSourcesFinished() const {
		size_t nFinished = atomic_load(&m_nSourcesFinished);
		assert(nFinished <= m_sources.size()); // Sanity check
		return nFinished;
	}

	bool allSourcesFinished() const { return nSourcesFinished() == m_sources.size(); }

public:
	const std::vector<Bric*>& sources() { return m_sources; }


// Dests //

protected:
	std::vector<Bric*> m_dests;

	std::atomic<size_t> m_nDestsReadyForInput;

	size_t m_outputCounter;


	void incNDestsReadyForInput() { atomic_fetch_add(&m_nDestsReadyForInput, size_t(1)); }
	void clearNDestsReadyForInput() { atomic_store(&m_nDestsReadyForInput, size_t(0)); }

	size_t nDestsReadyForInput() const {
		size_t nReady = atomic_load(&m_nDestsReadyForInput);
		assert(nReady <= m_dests.size()); // Sanity check
		return nReady;
	}

	bool allDestsReadyForInput() const { return nDestsReadyForInput() == m_dests.size(); }


	void announceNewOutput() {
		for (auto &dest: m_dests) dest->incNSourcesAvailable();
		clearNDestsReadyForInput();
		++m_outputCounter;
	}

	inline void setExecFinished() {
		assert(m_execFinished == false); // Sanity check
		m_execFinished = true;
		for (auto &dest: m_dests) dest->incSourcesFinished();
	}

public:
	const std::vector<Bric*>& dests() { return m_dests; }


// Execution //

public:

	// Must always be called for a whole set of interdependent sibling brics
	virtual void resetExec() {
		atomic_store(&m_nDestsReadyForInput, m_dests.size());
		m_execFinished = false;
	}

	// Returns true if execution is finished or new output was produced.
	// A repeated call, before any changes in higher/lower processing layers,
	// is guaranteed not to change the state of the bric and to return
	// true if bric execution is finished and false if not.
	bool nextExecStep() {
		if (!execFinished()) {
			return nextExecStepImpl();
		} else return true;
	}

	bool execFinished() const { return m_execFinished; }

public:
	friend class BricImpl;
	friend class BricWithInputs;
	friend class BricWithOutputs;
	friend class SyncedInputBric;
	friend class AsyncInputBric;
	friend class ProcessingBric;
	friend class ImportBric;
	friend class TransformBric;
	friend class MapperBric;
	friend class AbstractReducerBric;
	friend class ReducerBric;
	friend class AsyncReducerBric;
};



class BricImpl: public virtual Bric, public BricComponentImpl {
public:
	BricImpl() {}
	BricImpl(Name bricName): BricComponentImpl(bricName) {}
	BricImpl(Bric *parentBric, Name bricName)
		: BricComponentImpl(parentBric, bricName) { parentBric->registerBric(this); }
};



inline bool BricComponent::isInside(const Bric& other) const
	{ return hasParent() && (&parent() == &other || parent().isInside(other)); }


inline BricComponentImpl::BricComponentImpl(Bric *parentBric, Name componentName)
	: BricComponentImpl(componentName) { m_parent = parentBric; }



class BricWithOutputs: public virtual Bric  {
public:
	template <typename T> class Output
		: public virtual TypedOutputTerminal<T>, public BricComponentImpl, public HasTypedPrimaryValueImpl<T>
	{
	public:
		using HasTypedPrimaryValueImpl<T>::value;
		using HasTypedPrimaryValueImpl<T>::typeInfo;

		Output<T>& operator=(const Output<T>& v) = delete;

		Output<T>& operator=(const T &v)
			{ TypedOutputTerminal<T>::operator=(v); return *this; }

		Output<T>& operator=(T &&v) noexcept
			{ TypedOutputTerminal<T>::operator=(std::move(v)); return *this; }

		Output<T>& operator=(std::unique_ptr<T> &&v) noexcept
			{ TypedOutputTerminal<T>::operator=(std::move(v)); return *this; }


		Output(BricWithOutputs *parentBric)
			: BricComponentImpl(parentBric, s_defaultOutputName) { parentBric->registerOutput(this); }

		Output(BricWithOutputs *parentBric, Name outputName)
			: BricComponentImpl(parentBric, outputName) { parentBric->registerOutput(this); }

		template<typename U> Output(BricWithOutputs *parentBric, Name outputName, U&& defaultValue)
			: BricComponentImpl(parentBric, outputName)
		{
			parentBric->registerOutput(this);
			value() = std::forward<U>(defaultValue);
		}

		Output(const Output &other) = delete;
	};

	bool canHaveOutputs() const { return true; }
};



class BricWithInputs: public virtual Bric  {
public:
	template <typename T> class Input
		: public virtual TypedInputTerminal<T>, public BricComponentImpl, public HasTypedConstValueRefImpl<T>
	{
	protected:
		PropPath m_source;

	public:
		using HasTypedConstValueRefImpl<T>::value;
		using HasTypedConstValueRefImpl<T>::typeInfo;

		virtual void applyConfig(const PropVal& config) { m_source = config; }
		virtual PropVal getConfig() const  { return m_source; }

		virtual const PropPath& source() { return m_source; }

		//void connectTo(const TypedTerminal<T> &other) { value().referTo(other.value()); }

		void connectTo(const Terminal &other) { value().referTo(other.value()); }

		void connectTo(const Bric &bric, Name terminalName)
			{ connectTo(bric.getOutput(terminalName, typeInfo())); }

		void connectTo(const Bric &bric)
			{ connectTo(bric.getOutput(s_defaultOutputName, typeInfo())); }

		Input(BricWithInputs *parentBric, Name inputName)
			: BricComponentImpl(parentBric, inputName) { parentBric->registerInput(this); }

		Input(BricWithInputs *parentBric)
			: BricComponentImpl(parentBric, s_defaultInputName) { parentBric->registerInput(this); }

		Input(const Input &other) = delete;
	};

	bool canHaveInputs() const { return true; }
};



class SyncedInputBric: public virtual BricWithInputs {
protected:
	bool m_announcedReadyForInput = false;

	void announceReadyForInput() {
		if (!m_announcedReadyForInput && !allSourcesFinished()) {
			for (auto &source: m_sources) source->incNDestsReadyForInput();
			m_announcedReadyForInput = true;
		}
	}

	void consumeInput() {
		clearNSourcesAvailable();
		m_announcedReadyForInput = false;
	}

public:
	// User overload. Allowed to change output values.
	virtual void processInput() = 0;

	virtual void resetExec() {
		Bric::resetExec();
		m_announcedReadyForInput = true;
	}
};



class AsyncInputBric: public virtual BricWithInputs {
public:
	// User overload. Allowed to change output values.
	virtual void processInputFrom(const Bric *source) = 0;
};



class ProcessingBric: public virtual BricWithInputs, public virtual BricWithOutputs {
public:
};



class ImportBric: public virtual BricWithOutputs, public BricImpl {
protected:
	bool m_importDone = false;

public:
	// User overload. Allowed to change output values.
	virtual void import() = 0;

	void resetExec(){}

	bool nextExecStepImpl() {
		if (!m_importDone) {
			try{ import(); }
			catch(...) {
				dbrx_log_error("Running import failed in bric \"%s\"", absolutePath());
				setOutputsToErrorState();
				setExecFinished();
			}

			announceNewOutput();
			setExecFinished();

			// Import only once in the entire lifetime of the bric:
			m_importDone = true;
		}
		return true;
	}

public:
	using BricImpl::BricImpl;
};



// Is an export bric a useful concept? Maybe add later, if there is a need.
// class ExportBric: public virtual BricWithInputs { }



class TransformBric: public virtual ProcessingBric, public virtual SyncedInputBric, public BricImpl {
protected:
	bool nextExecStepImpl() {
		bool producedOutput = false;

		if (allDestsReadyForInput()) {
			assert(m_announcedReadyForInput); // Sanity check

			if (allSourcesAvailable()) {
				consumeInput();

				try{ processInput(); }
				catch(...) {
					dbrx_log_error("Processing input failed in bric \"%s\"", absolutePath());
					setOutputsToErrorState();
					setExecFinished();
				}

				announceNewOutput();
				producedOutput = true;
			} else {
				announceReadyForInput();
			}

			if (allSourcesFinished()) setExecFinished();
		}

		return producedOutput || execFinished();
	}

public:
	using BricImpl::BricImpl;
};



class MapperBric: public virtual ProcessingBric, public virtual SyncedInputBric, public BricImpl {
protected:
	bool m_readyForNextOutput = false;

	bool nextExecStepImpl() {
		bool producedOutput = false;

		if (allDestsReadyForInput()) {
			if (! m_readyForNextOutput) {
				assert(m_announcedReadyForInput); // Sanity check

				if (allSourcesAvailable()) {
					consumeInput();

					try{ processInput(); }
					catch(...) {
						dbrx_log_error("Processing input failed in bric \"%s\"", absolutePath());
						setOutputsToErrorState();
						setExecFinished();
					}

					m_readyForNextOutput = true;
				}
			}

			if (m_readyForNextOutput) {
				try{ producedOutput = nextOutput(); }
				catch(...) {
					dbrx_log_error("Producing next output failed in bric \"%s\"", absolutePath());
					setOutputsToErrorState();
					setExecFinished();
				}

				if (producedOutput) {
					announceNewOutput();
					m_readyForNextOutput = true;
				} else {
					announceReadyForInput();
				}
			} else {
				if (allSourcesFinished()) setExecFinished();
			}
		}

		return producedOutput || execFinished();
	}

public:
	// User overload. Allowed to change output values.
	virtual bool nextOutput() = 0;

	using BricImpl::BricImpl;
};



class AbstractReducerBric: public virtual ProcessingBric {
protected:
	bool m_reductionStarted = false;

	void beginReduction() {
		try { newReduction(); }
		catch(...) {
			dbrx_log_error("Initialization of reduction failed in bric \"%s\"", absolutePath());
			setOutputsToErrorState();
			setExecFinished();
		}

		m_reductionStarted = true;
	}

	bool reductionStarted() const { return m_reductionStarted; }

	void endReduction() {
		try { finalizeReduction(); }
		catch(...) {
			dbrx_log_error("Finalization of reduction failed in bric \"%s\"", absolutePath());
			setOutputsToErrorState();
			setExecFinished();
		}

		announceNewOutput();
		setExecFinished();
	}

public:
	// User overload. Allowed to change output values.
	virtual void newReduction() {}

	// User overload. Allowed to change output values.
	virtual void finalizeReduction() {}
};



class ReducerBric: public virtual AbstractReducerBric, public virtual SyncedInputBric, public BricImpl {
protected:
	void resetExec() {
		SyncedInputBric::resetExec();
		m_reductionStarted = false;
	}

	bool nextExecStepImpl() {
		if (m_reductionStarted == true) assert(allDestsReadyForInput()); // Sanity check

		if (allDestsReadyForInput()) {
			assert(m_announcedReadyForInput); // Sanity check

			if (allSourcesAvailable()) {
				if (! m_reductionStarted) beginReduction();
				consumeInput();

				try { processInput(); }
				catch(...) {
					dbrx_log_error("Processing input failed in bric \"%s\"", absolutePath());
					setOutputsToErrorState();
					setExecFinished();
				}

				announceReadyForInput();
			}

			if (allSourcesFinished()) endReduction();
		}

		return execFinished();
	}

public:
	using BricImpl::BricImpl;
};



class AsyncReducerBric: public virtual AbstractReducerBric, public virtual AsyncInputBric, public BricImpl {
protected:
	std::vector<size_t> m_inputCounter;

	void resetExec() {
		ProcessingBric::resetExec();
		m_reductionStarted = false;
		m_inputCounter.clear();
		m_inputCounter.resize(m_sources.size());
	}

	bool nextExecStepImpl() {
		if (m_reductionStarted == true) assert(allDestsReadyForInput()); // Sanity check

		if (allDestsReadyForInput()) {
			bool gotInput = false;
			for (size_t i = 0; i < m_sources.size(); ++i) {
				auto &source = m_sources[i];
				if (m_inputCounter[i] < source->m_outputCounter) {
					if (! m_reductionStarted) beginReduction();
					decNSourcesAvailable();
					m_inputCounter[i] = source->m_outputCounter;
					gotInput = true;

					try { processInputFrom(source); }
					catch(...) {
						dbrx_log_error("Processing input from source \"%s\" failed in bric \"%s\"", source->absolutePath(), absolutePath());
						setOutputsToErrorState();
						setExecFinished();
					}

					source->incNDestsReadyForInput();
				}
			}

			if (allSourcesFinished()) endReduction();
		}

		return execFinished();
	}
public:
	using BricImpl::BricImpl;
};


} // namespace dbrx


#endif // DBRX_BRIC_H
