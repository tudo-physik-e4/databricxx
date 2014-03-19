#ifndef databricxx_LinkDef_h
#define databricxx_LinkDef_h

#ifdef __CINT__

// Bric.h
#pragma link C++ class dbrx::Bric-;
#pragma link C++ class dbrx::Bric::Terminal-;
#pragma link C++ class dbrx::Bric::OutputTerminal-;
#pragma link C++ class dbrx::Bric::InputTerminal-;
#pragma link C++ class dbrx::BricImpl-;
#pragma link C++ class dbrx::BricWithOutputs-;
#pragma link C++ class dbrx::BricWithInputs-;
#pragma link C++ class dbrx::BricWithInOut-;
#pragma link C++ class dbrx::InputBric-;
#pragma link C++ class dbrx::OutputBric-;
#pragma link C++ class dbrx::FilterBric-;
#pragma link C++ class dbrx::MapperBric-;
#pragma link C++ class dbrx::ReducerBric-;

// DbrxTools.h
#pragma link C++ class dbrx::DbrxTools-;

// Name.h
#pragma link C++ class dbrx::Name-;
#pragma link C++ class dbrx::HasName-;
#pragma link C++ class dbrx::HasNameImpl-;

// Prop.h
#pragma link C++ class dbrx::Prop-;

// Reflection.h
#pragma link C++ class dbrx::Reflection-;

// RootIO.h
#pragma link C++ class dbrx::RootIO-;

// Value.h, HasValue.h
#pragma link C++ class dbrx::Value-;
#pragma link C++ class dbrx::WritableValue-;
#pragma link C++ class dbrx::UniqueValue-;
#pragma link C++ class dbrx::ValueRef-;
#pragma link C++ class dbrx::ConstValueRef-;

#pragma link C++ class dbrx::HasValue-;
#pragma link C++ class dbrx::HasWritableValue-;
#pragma link C++ class dbrx::HasUniqueValue-;
#pragma link C++ class dbrx::HasValueRef-;
#pragma link C++ class dbrx::HasConstValueRef-;

// brics.h

#endif // __CINT__

#endif // databricxx_LinkDef_h
