/* -------------------------------------------------------------------------------
* Copyright (c) 2020, OLogN Technologies AG
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the OLogN Technologies AG nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* -------------------------------------------------------------------------------*/

#ifndef NODECPP_NODECPP_ERROR_H
#define NODECPP_NODECPP_ERROR_H 

#include <platform_base.h>
#include <error.h>
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_PTR_LIFECYCLE_INFO
#include <stack_info.h>
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_PTR_LIFECYCLE_INFO
#include <fmt/format.h>

#include <cerrno>  // for error constants

using namespace nodecpp::error;

namespace nodecpp::error {

	enum class NODECPP_EXCEPTION { null_ptr_access };

	class nodecpp_error_value : public error_value
	{
		friend class nodecpp_error_domain;
		NODECPP_EXCEPTION errorCode;
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_PTR_LIFECYCLE_INFO
		::nodecpp::StackInfo stackInfo;
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_PTR_LIFECYCLE_INFO
		string_ref extra;
	public:
		nodecpp_error_value( NODECPP_EXCEPTION code ) : errorCode( code ), extra( string_ref::literal_tag_t(), "" ) { 
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_PTR_LIFECYCLE_INFO
			stackInfo.init();
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_PTR_LIFECYCLE_INFO
		}
		nodecpp_error_value( NODECPP_EXCEPTION code, string_ref&& extra_ ) : errorCode( code ), extra( std::move( extra_ ) ) {
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_PTR_LIFECYCLE_INFO
			stackInfo.init();
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_PTR_LIFECYCLE_INFO
		}
		nodecpp_error_value( const nodecpp_error_value& other ) = default;
		nodecpp_error_value& operator = ( const nodecpp_error_value& other ) = default;
		nodecpp_error_value& operator = ( nodecpp_error_value&& other ) = default;
		nodecpp_error_value( nodecpp_error_value&& other ) = default;
		virtual ~nodecpp_error_value() {}
	};

	class nodecpp_error_domain : public error_domain
	{
	protected:
		virtual uintptr_t _nodecpp_get_error_code(const error_value* value) const { return (int)(reinterpret_cast<const nodecpp_error_value*>(value)->errorCode); } // for inter-domain comparison purposes only

	public:
		constexpr nodecpp_error_domain() {}
		using Valuetype = nodecpp_error_value;
		virtual string_ref name() const { return string_ref( string_ref::literal_tag_t(), "nodecpp error" ); }
		virtual string_ref value_to_message(error_value* value) const { 
			nodecpp_error_value* myData = reinterpret_cast<nodecpp_error_value*>(value);
			switch ( myData->errorCode )
			{
				case NODECPP_EXCEPTION::null_ptr_access:
				{
					std::string s;
					if ( !myData->extra.empty() )
					{
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_PTR_LIFECYCLE_INFO
						if ( ::nodecpp::impl::isDataStackInfo( myData->stackInfo ) )
							s = fmt::format("Attempt to dereference a null pointer at\n{}\n{}", ::nodecpp::impl::whereTakenStackInfo( myData->stackInfo ).c_str(), myData->extra.c_str());
						else
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_PTR_LIFECYCLE_INFO
							s = fmt::format("Attempt to dereference a null pointer\n{}", myData->extra.c_str());
					}
					else
					{
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_PTR_LIFECYCLE_INFO
						if ( ::nodecpp::impl::isDataStackInfo( myData->stackInfo ) )
							s = fmt::format("Attempt to dereference a null pointer at\n{}", ::nodecpp::impl::whereTakenStackInfo( myData->stackInfo ).c_str() );
						else
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_PTR_LIFECYCLE_INFO
							s = fmt::format("Attempt to dereference a null pointer" );
					}
					return string_ref( s.c_str() );
				}
				default: return "unknown nodecpp error";
			}
		}
		virtual void log(error_value* value, log::LogLevel l ) const { log::default_log::log( l, "{}", value_to_message( value ).c_str() ); }
		virtual void log(error_value* value, log::Log& targetLog, log::LogLevel l ) const { targetLog.log( l, "{}", value_to_message( value ).c_str() ); }
		error_value* create_value( NODECPP_EXCEPTION code, string_ref&& extra ) const {
			return new nodecpp_error_value(code, std::move( extra ));
		}
		virtual bool is_same_error_code(const error_value* value1, const error_value* value2) const { 
			return reinterpret_cast<const nodecpp_error_value*>(value1)->errorCode == reinterpret_cast<const nodecpp_error_value*>(value2)->errorCode;
		}
		virtual error_value* clone_value(error_value* value) const {
			if ( value )
				return new nodecpp_error_value( *reinterpret_cast<nodecpp_error_value*>(value) );
			else
				return nullptr;
		}
		virtual void destroy_value(error_value* value) const {
			if ( value ) {
				nodecpp_error_value* myData = reinterpret_cast<nodecpp_error_value*>(value);
				delete myData;
			}
		}
		virtual bool is_equivalent( const error& src, const error_value* my_value ) const {
			if ( src.domain() == this )
				return is_same_error_code( my_value, src.value() );
			else
				return false;
		}
	};
	extern const nodecpp_error_domain nodecpp_error_domain_obj;

	class nodecpp_error : public error
	{
	public:
		using DomainTypeT = nodecpp_error_domain;
		nodecpp_error(NODECPP_EXCEPTION code, string_ref&& extra) : error( &nodecpp_error_domain_obj, nodecpp_error_domain_obj.create_value(code, std::move( extra )) ) {}
	};

} // namespace nodecpp::error


#endif // NODECPP_NODECPP_ERROR_H
