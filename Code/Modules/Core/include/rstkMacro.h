// --------------------------------------------------------------------------------------
// File:          rstkMacro.h
// Date:          Jan 31, 2014
// Author:        code@oscaresteban.es (Oscar Esteban)
// Version:       1.5.5
// License:       GPLv3 - 29 June 2007
// Short Summary:
// --------------------------------------------------------------------------------------
//
// Copyright (c) 2014, code@oscaresteban.es (Oscar Esteban)
// with Signal Processing Lab 5, EPFL (LTS5-EPFL)
// and Biomedical Image Technology, UPM (BIT-UPM)
// All rights reserved.
//
// This file is part of RegSeg
//
// RegSeg is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RegSeg is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RegSeg.  If not, see <http://www.gnu.org/licenses/>.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef RSTKMACRO_H_
#define RSTKMACRO_H_

#include <itkMacro.h>


/*
    if ( this->m_##name.size() <= id ) { \
		itkExceptionMacro( << "vector " #name "is not initialized, or position " << id << " not valid."); \
	} \
 */


#define rstkSetVectorElement(name,type) \
virtual void Set##name##Element ( size_t id, const type _arg ) { \
	if ( this->m_##name.size() <= id ) { \
		itkExceptionMacro( << "vector " #name " is not initialized, or position " << id << " not valid."); \
	} \
	if ( this->m_##name[id] != _arg ) { \
		this->m_##name[id] = _arg; \
		this->Modified(); \
	} \
}

#define rstkGetVectorElement(name,type) \
virtual type Get##name##Element ( size_t id ) { \
	return this->m_##name[id]; \
}

#define rstkGetConstVectorElement( name, type ) \
virtual type Get##name##Element( size_t id ) const { \
	return this->m_##name[id]; \
}

#define rstkFillVector(name,type) \
virtual void Fill##name ( const type _arg ) { \
	for( size_t id = 0; id < this->m_##name[id]; id++) { \
		if ( this->m_##name[id] != _arg ) { \
			this->m_##name[id] = _arg; \
			this->Modified(); \
		} \
	} \
}

#define rstkVectorMethods(name,type) \
virtual void Set##name##Element ( size_t id, const type _arg ) { \
	if ( this->m_##name.size() <= id ) { \
		itkExceptionMacro( << "vector " #name " is not initialized, or position " << id << " not valid."); \
	} \
	if ( this->m_##name[id] != _arg ) { \
		this->m_##name[id] = _arg; \
		this->Modified(); \
	} \
} \
virtual type Get##name##Element( const size_t id ) const { \
	return this->m_##name[id]; \
} \
virtual void Fill##name ( const type _arg ) { \
	for( size_t id = 0; id < this->m_##name[id]; id++) { \
		if ( this->m_##name[id] != _arg ) { \
			this->m_##name[id] = _arg; \
			this->Modified(); \
		} \
	} \
}



#define rstkGetObjectList(name, type) \
virtual type * Get##name##OfLevel ( const size_t id ) const  { \
    return this->m_##name##s[id].GetPointer();    \
}

#define rstkGetObjectListWithLast(name, type) \
virtual type * Get##name##OfLevel ( const int id ) const  { \
	if ( this->m_##name##s.empty() ) { \
		itkExceptionMacro(<< "list " #name " is empty." );\
	} \
	size_t real_id = id; \
	if( id == -1 ) { \
		real_id = this->m_##name##s.size() -1; \
	} else if ( id < -1 || id > this->m_##name##s.size() ) { \
		itkExceptionMacro(<< "attempted to access invalid position in list " #name "." );\
	} \
    return this->m_##name##s[real_id].GetPointer(); \
}


#define rstkSetObjectList(name, type) \
virtual void Set##name##OfLevel ( const size_t id, type* obj )  { \
	if ( this->m_##name##s.size() <= id ) { \
		itkExceptionMacro( << "vector " #name " is not initialized, or position " << id << " not valid."); \
	} \
	if ( this->m_##name##s[id] != _arg ) { \
		this->m_##name##s[id] = _arg; \
		this->Modified(); \
	} \
}

/** Get a smart const pointer to an object.  Creates the member
 * Get"name"() (e.g., GetPoints()). */
#define rstkGetConstObjectList(name, type) \
virtual const type * Get##name##OfLevel ( const size_t id ) const  { \
    return this->m_##name##s[id].GetPointer();    \
}
#endif /* RSTKMACRO_H_ */
