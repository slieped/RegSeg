/*
 * IterationUpdate.h
 *
 *  Created on: Oct 5, 2013
 *      Author: oesteban
 */

#ifndef ITERATIONUPDATE_H_
#define ITERATIONUPDATE_H_

#include <itkCommandIterationUpdate.h>
#include <itkWeakPointer.h>
#include "OptimizerBase.h"


namespace rstk {

template< typename TOptimizer >
class IterationUpdate: public itk::CommandIterationUpdate<TOptimizer>
{
public:
	typedef IterationUpdate                           Self;
	typedef TOptimizer                                OptimizerType;
	typedef typename itk::WeakPointer<OptimizerType>  OptimizerPointer;
	typedef itk::CommandIterationUpdate<TOptimizer>   Superclass;
	typedef itk::SmartPointer<Self>                   Pointer;
	typedef itk::SmartPointer< const Self >           ConstPointer;

	itkTypeMacro( IterationUpdate, itk::CommandIterationUpdate ); // Run-time type information (and related methods)
	itkNewMacro( Self );

    void Execute(itk::Object *caller, const itk::EventObject & event) {
        Execute( (const itk::Object *)caller, event);
    }

    void Execute(const itk::Object * object, const itk::EventObject & event) {

    	if( typeid( event ) == typeid( itk::IterationEvent ) ) {
    		std::cout << "[" << this->m_Optimizer->GetCurrentIteration() << "] ";
    		if( !this->m_Optimizer->GetUseLightWeightConvergenceChecking() ) {
    			std::cout << this->m_Optimizer->GetCurrentEnergy() << " | " << this->m_Optimizer->GetFunctional()->GetValue() << " | " << this->m_Optimizer->GetCurrentRegularizationEnergy();
    		}
    		std::cout << "\t" << this->m_Optimizer->GetCurrentValue() << std::endl;
    	}
    }

    void SetOptimizer( OptimizerType * optimizer ) {
      m_Optimizer = optimizer;
      m_Optimizer->AddObserver( itk::IterationEvent(), this );
    }

    itkSetMacro( Level, size_t );

protected:
	IterationUpdate(): m_Level(0) {}
	~IterationUpdate(){}

	//void PrintSelf( std::ostream &os, itk::Indent indent ) const;
	size_t m_Level;
private:
	IterationUpdate( const Self & ); // purposely not implemented
	void operator=( const Self & ); // purposely not implemented


	OptimizerPointer   m_Optimizer;
};

} // end namespace rstk


#endif /* ITERATIONUPDATE_H_ */
