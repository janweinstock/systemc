/**********************************************************************
  The following code is derived, directly or indirectly, from the SystemC
  source code Copyright (c) 1996-2008 by all Contributors.
  All Rights reserved.

  The contents of this file are subject to the restrictions and limitations
  set forth in the SystemC Open Source License Version 3.0 (the "License");
  You may not use this file except in compliance with such restrictions and
  limitations. You may obtain instructions on how to receive a copy of the
  License at http://www.systemc.org/. Software distributed by Contributors
  under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
  ANY KIND, either express or implied. See the License for the specific
  language governing rights and limitations under the License.
 *********************************************************************/
//=====================================================================
///  @file memory.cpp
//
///  @Details
///  
//
//=====================================================================
//  Original Authors:
//    Jack Donovan, ESLX
//=====================================================================

#include "reporting.h"                      // Reporting convenience macros
#include "memory.h"                         // Our header

using namespace sc_core;

static const char *filename = "memory.cpp"; ///< filename for reporting

/// Constructor

memory::memory    
(
  const unsigned int ID                  // Target ID
, sc_core::sc_time   read_delay          // read delay
, sc_core::sc_time   write_delay         // write delay
, sc_dt::uint64      memory_size         // memory size (bytes)
, unsigned int       memory_width        // memory width (bytes)
)
: m_ID              (ID)                        
, m_read_delay      (read_delay)           
, m_write_delay     (write_delay)
, m_memory_size     (memory_size)
, m_memory_width    (memory_width)
, m_previous_warning(false)     
{ 
/// Allocate and initalize an array for the target's memory
  m_memory = new unsigned char[size_t(m_memory_size)];

/// clear memory
  memset(m_memory, 0, size_t(m_memory_size));              

} // end Constructor

//=====================================================================
///  @fn memory::memory_operation
//  
///  @brief helper function for read and write processing
// 
///  @details
///    This routine implements the read and  write operations
//   
//=====================================================================
void 
memory::operation     
( 
    tlm::tlm_generic_payload  &gp     
  , sc_core::sc_time          &delay_time   ///< transaction delay 
)    
{
  /// Access the required attributes from the payload
  sc_dt::uint64    address   = gp.get_address();     // memory address
  tlm::tlm_command command   = gp.get_command();     // memory command
  unsigned char    *data     = gp.get_data_ptr();    // data pointer
  unsigned  int     length   = gp.get_data_length(); // data length
  
  std::ostringstream  msg;   
  msg.str("");
  tlm::tlm_response_status response_status = check_address(gp);
  ///@todo we probably need to not proces command and do the "correct thing"
  /// immediately
//    if (gp.get_byte_enable_ptr()){
//    gp.set_response_status(tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE);
//  }
//  else if (gp.get_streaming_width()) {
//    gp.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
//  }
  switch (command)
  {
    default:
    {
      // ONe time warning message
      ///@todo Warning for example but should refer to ignorable extension
      ///@todo change to Info
      ///@todo choose appropriate response status TLM_COMMAND_ERROR ????
      ///@todo what delay?
      if (m_previous_warning == false)
      {
        msg << "Target: " << m_ID 
            << " Invalid GP command";
        REPORT_WARNING(filename, __FUNCTION__, msg.str());
        m_previous_warning = true;
      }
      break;
    }
    
    /// Setup a TLM_WRITE_COMMAND Informational Message and Write the Data from
    /// the Generic Payload Data pointer to Memory
    ///
    case tlm::TLM_WRITE_COMMAND:
    {
      if (response_status == tlm::TLM_OK_RESPONSE)
        {
          for (unsigned int i = 0; i < length; i++)
          {
            m_memory[address++] = data[i];     // move the data to memory
          }
          delay_time = delay_time + m_write_delay;
          report::print(m_ID, gp);
        } 
      break;     
    }

    case tlm::TLM_READ_COMMAND:
    {
      if (response_status == tlm::TLM_OK_RESPONSE)
      {
        for (unsigned int i = 0; i < length; i++)
        {
          data[i] = m_memory[address++];         // move the data to memory
        }
        delay_time = delay_time + m_read_delay;
        report::print(m_ID, gp);
      }     
      break;
    }
  } // end switch

  gp.set_response_status(response_status);
  
  return;
} // end memory_operation

unsigned char* 
memory::get_mem_ptr(void)
  {
    return m_memory;
  }

void 
memory::get_delay
(
   tlm::tlm_generic_payload  &gp           ///< TLM2 GP reference
 , sc_core::sc_time          &delay_time   ///< time to be updated
)
{
  /// Access the required attributes from the payload
  tlm::tlm_command command   = gp.get_command();     // memory command
  
  std::ostringstream  msg;   
  msg.str("");
  
  switch (command)
  {
    default:
    {
      ///@todo need to modify this to match operation method
      // One time warning message
      if (m_previous_warning == false)
      {
        msg << "Target: " << m_ID 
            << " Invalid GP command";
        REPORT_WARNING(filename, __FUNCTION__, msg.str());
        m_previous_warning = true;
      }
      break;
    }
    
    /// Setup a TLM_WRITE_COMMAND Informational Message and Write the Data from
    /// the Generic Payload Data pointer to Memory
    ///
    case tlm::TLM_WRITE_COMMAND:
    {
      delay_time = delay_time + m_write_delay;
      break;     
    }

    case tlm::TLM_READ_COMMAND:
    {
      delay_time = delay_time + m_read_delay;
      break;
    }
  } // end switch  
  return; 
}


tlm::tlm_response_status
memory::check_address
(
  tlm::tlm_generic_payload  &gp        
)
{
  sc_dt::uint64    address   = gp.get_address();     // memory address
  unsigned  int     length   = gp.get_data_length(); // data length
  
  std::ostringstream  msg;   
  msg.str("");
  
  if  ( (address < 0) || (address >= m_memory_size) )            
  {
    msg << "Target: " << m_ID 
        <<" address out-of-range";
    REPORT_WARNING(filename, __FUNCTION__, msg.str());
    
    return tlm::TLM_ADDRESS_ERROR_RESPONSE; // operation response
  }
  else
  {
    if ( (address + length) >= m_memory_size )   
    {
      msg << "Target: " << m_ID 
          << " address will go out of bounds";
      REPORT_WARNING(filename, __FUNCTION__, msg.str());
      
      return tlm::TLM_ADDRESS_ERROR_RESPONSE; // operation response
    }
    
  return tlm::TLM_OK_RESPONSE;
  }
} // end check address


 
