
#include "processor.h"

void processor::trace_register_read (unsigned int pe_number, unsigned int lane_number)
{
   unsigned int trace_index;
   unsigned int inst_index;
   unsigned int index;
   unsigned int pe = pe_number;
   db_t *actual;
   
   if(TR_PAY.is_empty())
	return;

   if(TR_AL->is_empty() == true)
	return;

   // Check if there is an instruction in the Register Read Stage of the specified Execution Lane.
   if (PE_array[pe]->Execution_Lanes[lane_number].rr.valid)
   {

      //////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Get the instruction's index into PAY.
      //////////////////////////////////////////////////////////////////////////////////////////////////////////
      index = PE_array[pe]->Execution_Lanes[lane_number].rr.index;

      //////////////////////////////////////////////////////////////////////////////////////////////////////////
      // +Parth
      // If the instruction has a destination register:
      // (1) Broadcast its destination tag to the appropriate IQ to wakeup its dependent instructions.
      // Appropriate IQ refers to either INT/FP or global/local IQ.
      // (2) Set the corresponding ready bit in the global/local Physical Register File Ready Bit Array.
      //////////////////////////////////////////////////////////////////////////////////////////////////////////
 
      // Not load instr
      if (!IS_LOAD(PAY.buf[index].flags))
      {
         if (PAY.buf[index].C_valid == true)
         {
         	  // INT destination register.
               if (PAY.buf[index].C_int == true)
               {
             	  if (PAY.buf[index].C_live_out == true)
             	  {
                     //Global wakeup and GRF ready bit set
                     for (int i=0; i<TOTAL_PE; i++)
                     {
                        if (PE_array[i]->valid == true && i!=pe)
                        {
                           PE_array[i]->IQ_INT.global_wakeup(PAY.buf[index].C_global_phys_reg);
                        }
                     }
                     
                     REN_INT->set_ready(PAY.buf[index].C_global_phys_reg);
             	  }
             	  
                 if (PAY.buf[index].C_local == true)
             	  {
                     //Local wakeup and LRF ready bit set
                     PE_array[pe]->IQ_INT.local_wakeup(PAY.buf[index].C_local_phys_reg);
                     PE_array[pe]->set_ready_int(PAY.buf[index].C_local_phys_reg);
             	  }
               }

               //FP destination register.
               else if(PAY.buf[index].C_int == false)
               {
                 if (PAY.buf[index].C_live_out == true)
                 {
                     //Global wakeup and GRF ready bit set
                     for (int i=0; i<TOTAL_PE; i++)
                     {
                        if (PE_array[i]->valid == true && i!=pe)
                        {
                           PE_array[i]->IQ_FP.global_wakeup(PAY.buf[index].C_global_phys_reg);
                        }
                     }
                     REN_FP->set_ready(PAY.buf[index].C_global_phys_reg);
                 }
                 
                 if (PAY.buf[index].C_local == true)
                 {
                     //Local wakeup and LRF ready bit set
                     PE_array[pe]->IQ_FP.local_wakeup(PAY.buf[index].C_local_phys_reg);
                     PE_array[pe]->set_ready_fp(PAY.buf[index].C_local_phys_reg);
                 }
               }
         }
      }

      //////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Read source register(s) from the appropriate Physical Register File.
      //////////////////////////////////////////////////////////////////////////////////////////////////////////

      // Source register A
      if(PAY.buf[index].A_valid == true)
      {
            //INT source register
            if(PAY.buf[index].A_int == true)
            {
               if(PAY.buf[index].A_live_in == true)
               {
                  PAY.buf[index].A_value.dw = REN_INT->read(PAY.buf[index].A_global_phys_reg);
		  /*if (PAY.buf[index].A_log_reg == 29 && PAY.buf[index].good_instruction = true && num_traces >= 30) 
		  {
              	      actual = THREAD[Tid]->peek(PAY.buf[index].db_index);
			PAY.buf[index].A_value.w[0] = actual->a_rsrc[0].value;
			//PAY.buf[index].A_value.w[1] = actual->a_rsrc[1].value;
	              if ( ( SS_OPCODE(PAY.buf[index].inst) == DLW ) || (SS_OPCODE(PAY.buf[index].inst) == DSW) || (SS_OPCODE(PAY.buf[index].inst) == S_S) || (SS_OPCODE(PAY.buf[index].inst) == S_D) )
		 	{ PAY.buf[index].A_value.w[0] = actual->a_rsrcA[0].value;  PAY.buf[index].A_value.w[1] = actual->a_rsrcA[1].value;	}
		  }*/
               }

               else if (PAY.buf[index].A_local == true)
               {
                  PAY.buf[index].A_value.dw = PE_array[pe]->read_int(PAY.buf[index].A_local_phys_reg);
		  /*if (PAY.buf[index].A_log_reg == 29 && PAY.buf[index].good_instruction = true && num_traces >= 30)
	          {
              	      actual = THREAD[Tid]->peek(PAY.buf[index].db_index);
			PAY.buf[index].A_value.w[0] = actual->a_rsrc[0].value;
			//PAY.buf[index].A_value.w[0] = actual->a_rsrc[0].value;
	              if ( (SS_OPCODE(PAY.buf[index].inst) == DLW) || (SS_OPCODE(PAY.buf[index].inst) == DSW) || (SS_OPCODE(PAY.buf[index].inst) == S_S) || (SS_OPCODE(PAY.buf[index].inst) == S_D) )
			 { PAY.buf[index].A_value.w[0] = actual->a_rsrcA[0].value;  PAY.buf[index].A_value.w[1] = actual->a_rsrcA[1].value;	}	 
		  }*/               
               }                  
            }

            //FP source register
            else if(PAY.buf[index].A_int == false)
            {
               if(PAY.buf[index].A_live_in == true)
               {
                  PAY.buf[index].A_value.dw = REN_FP->read(PAY.buf[index].A_global_phys_reg);
               }

               else if (PAY.buf[index].A_local == true)
               {
                  PAY.buf[index].A_value.dw = PE_array[pe]->read_fp(PAY.buf[index].A_local_phys_reg);                 
               } 
            }    
      }

      // Source register B
      if(PAY.buf[index].B_valid == true)
      {
            //INT source register
            if(PAY.buf[index].B_int == true)
            {
               if(PAY.buf[index].B_live_in == true)
               {
                  PAY.buf[index].B_value.dw = REN_INT->read(PAY.buf[index].B_global_phys_reg);
               }

               else if (PAY.buf[index].B_local == true)
               {
                  PAY.buf[index].B_value.dw = PE_array[pe]->read_int(PAY.buf[index].B_local_phys_reg);                 
               }                  
            }

            //FP source register
            else if(PAY.buf[index].B_int == false)
            {
               if(PAY.buf[index].B_live_in == true)
               {
                  PAY.buf[index].B_value.dw = REN_FP->read(PAY.buf[index].B_global_phys_reg);
               }

               else if (PAY.buf[index].B_local == true)
               {
                  PAY.buf[index].B_value.dw = PE_array[pe]->read_fp(PAY.buf[index].B_local_phys_reg);                 
               } 
            }    
      }

      //////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Advance the instruction to the Execution Stage.
      //////////////////////////////////////////////////////////////////////////////////////////////////////////

      // There must be space in the Execute Stage because Execution Lanes are free-flowing.
      assert(!PE_array[pe]->Execution_Lanes[lane_number].ex.valid);

      // Copy instruction to Execute Stage.
      PE_array[pe]->Execution_Lanes[lane_number].ex.valid = true;
      PE_array[pe]->Execution_Lanes[lane_number].ex.index = PE_array[pe]->Execution_Lanes[lane_number].rr.index;

      // Remove instruction from Register Read Stage.
      PE_array[pe]->Execution_Lanes[lane_number].rr.valid = false;
   }
}
