/*
 * Debug_HAL.c
 *
 *  Created on: ??�/??�/????
 *      Author: YUSSRY
 */



/*
*********************************************************************************************************
*                                     MICIRUM BOARD SUPPORT PACKAGE
*
*                             (c) Copyright 2012; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                        BOARD SUPPORT PACKAGE
*
*                                         Xilinx Zynq XC7Z020
*                                               on the
*
*                                             Xilinx ZC702
*                                          Evaluation Board
*
* Filename      : bsp.c
* Version       : V1.00
* Programmer(s) : JPB
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#define Debug_HAL_IMPORT
#include "Debug_HAL.h"



/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

 _gdb_arch arch_gdb_ops = {
	.gdb_bpt_instr		=  {0xfe, 0xde, 0xff, 0xe7}  //Little-Endian
};

/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/
 static Debug_MemWidth Get_Target_DP_Class(Debug_MemWidth Instruction);

 static Debug_MemWidth Get_Target_LDSTR_Class(Debug_MemWidth Instruction);

 static Debug_MemWidth Get_Target_Branch_Class(Debug_MemWidth Instruction);

 static Debug_MemWidth Get_Target_COPSVC_Class(Debug_MemWidth Instruction);

 static Debug_MemWidth Get_Target_LDSTR_Media(Debug_MemWidth Instruction);
 static Debug_MemWidth Get_Target_UNCOND_Class(Debug_MemWidth Instruction);
 static Debug_MemWidth Shift_Shift_C(Debug_MemWidth operand, CPU_INT08U shift_t, CPU_INT08U shift_n);
 static CPU_INT08U Count_NumRegsLoaded( CPU_INT16U CPU_RegList);



/*
*********************************************************************************************************
*                                             REGISTERS
*********************************************************************************************************
*/




/*
*********************************************************************************************************
*                                            REGISTER BITS
*********************************************************************************************************
*/




/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/





/*
*********************************************************************************************************
*                                               void Debug_HAL_init()
*
* Description :
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : .
*
* Note(s)     : (1)
*
*               (2)
*********************************************************************************************************
*/

void Debug_HAL_init(void)
{

	/*Initialize Registers Buffer*/
	CPU_INT08U RegOffset;
	for(RegOffset =R0_Offset ; RegOffset< Debug_HAL_Regs_IDMAX ; RegOffset++)
		Debug_HAL_RegsBuffer[RegOffset] = 0;

}


/*
*********************************************************************************************************
*                                       Register Functions
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               void Debug_HAL_Regs_Readall()
*
* Description :Reads target registers that GDB needs in <g> command packet . Registers are Thread-specific and is stored in Debug_HAL_RegsBuffer[]
*
* Argument(s) : ThreadID : thread id for which this function read its register context.
*
* Return(s)   : none.
*
* Caller(s)   : Debug_Main_Read_Registers() in Debug_Main Module
*
* Note(s)     : (1) ThreadID could be thread priority , thread ID  depending on the target system(RTOS)
*
*               (2)refer to OSTaskStkInit() in os_cpu_c.c to know how registers are stored in task stack.
*********************************************************************************************************
*/

void Debug_HAL_Regs_Readall(Debug_TID_t ThreadID)
{

	/*ThreadID here means priority*/

	CPU_INT08U count ;
	//Debug_TID_t current_thread = current_thread_OF_Focus;

	Debug_TCB *Deubg_TCBCurr = (Debug_TCB *)Debug_RTOS_ThreadListPtr[ThreadID] ; /*get a pointer to the thread stack from the Task Control Block */
	Debug_RegisterWidth *Debug_SP = Deubg_TCBCurr->OSTCBStkPtr;

	/***************Note : Debug_SP is incremented or decremented based on stack-growth setting"OS_STK_GROWTH macro" in os_cpu.h *********************/
	Debug_HAL_RegsBuffer[SP_Offset] = Debug_SP;                                        /*Get the SP*/
	Debug_HAL_RegsBuffer[CPSR_Offset] = *(Debug_SP++);                                 /*Get the CPSR*/

	for(count = R0_Offset; count <= R12_Offset;count++)                                    /*get R0-R12*/
		Debug_HAL_RegsBuffer[count] = *(Debug_SP++);

	Debug_HAL_RegsBuffer[LR_Offset] = *(Debug_SP++);                                       /*Get R14=LR , R13=SP is skipped
	                                                                                     as it is not stored in task stack*/

	Debug_HAL_RegsBuffer[PC_Offset] = *(Debug_SP);                                       /*Get R15=PC */


}

/*
*********************************************************************************************************
*                                                Debug_HAL_Regs_Writeall()
*
* Description : Writes target registers with GDB values in <G> command packet . Registers values
*               are Thread-specific and is stored in Debug_HAL_RegsBuffer[]
*
* Argument(s) : ThreadID : thread id for which this function modify its register context.
*
* Return(s)   : none.
*
* Caller(s)   : Debug_Main_Write_Registers() in Debug_Main Module
*
* Note(s)     : (1)ThreadID could be thread priority , thread ID  depending on the target system(RTOS)
*
*               (2)refer to OSTaskStkInit() in os_cpu_c.c to know how registers are stored in task stack.
*********************************************************************************************************
*/
void Debug_HAL_Regs_Writeall(Debug_TID_t ThreadID)
{
	CPU_INT08U count = 0 ;
	//Debug_TID_t current_thread = current_thread_OF_Focus;

	Debug_TCB *Deubg_TCBCurr = (Debug_TCB *)Debug_RTOS_ThreadListPtr[ThreadID] ; /*get a pointer to the thread stack from the Task Control Block */
	Debug_RegisterWidth *Debug_SP = Deubg_TCBCurr->OSTCBStkPtr;

	/***************Note : Debug_SP is incremented or decremented based on stack-growth setting"OS_STK_GROWTH macro" in os_cpu.h *********************/


	*(Debug_SP++) = Debug_HAL_RegsBuffer[CPSR_Offset];                                 /*set the CPSR*/

	for(count = R0_Offset ; count <= R12_Offset ; count++)                                    /*set R12-R0*/
		*(Debug_SP++) = Debug_HAL_RegsBuffer[count] ;

	Deubg_TCBCurr->OSTCBStkPtr = Debug_HAL_RegsBuffer[SP_Offset];                                 /*set the R13 = SP*/

	*(Debug_SP++) = Debug_HAL_RegsBuffer[LR_Offset] ;                                       /*set R14=LR , R13=SP is skipped
		                                                                                     as it is not stored in task stack*/

	*(Debug_SP++) = Debug_HAL_RegsBuffer[PC_Offset]  ;                                       /*set R15=PC */






}

/*
*********************************************************************************************************
*                                                Debug_HAL_Regs_ReadOne()
*
* Description :Read one register from Thread register context
*
* Argument(s) : ThreadID : thread id for which this function get  register regno.
*               regno    : Register ID as specified by GDB in <p> command packet.
*               RegValue : The returned/read register value.
*
* Return(s)   : DEBUG_SUCCESS : if no error
*               DEBUG_ERR_INVALID_REGID if register ID "regno" is not valid.
*
* Caller(s)   :
*
* Note(s)     : (1)Debug_Main_Read_Register() in Debug_Main module
*
*               (2)
*********************************************************************************************************
*/
CPU_INT08U Debug_HAL_Regs_ReadOne(Debug_TID_t ThreadID,Debug_RegID_t regno ,Debug_RegisterWidth *RegValue )
{

	/***************Note : Debug_SP is incremented or decremented based on stack-growth setting"OS_STK_GROWTH macro" in os_cpu.h *********************/

	/*
	*********************************************************************************************************
	*                                             1st Method to read one register
	*********************************************************************************************************
	*/
	if(regno > Debug_HAL_Regs_IDMAX)
		return DEBUG_ERR_INVALID_REGID;

	Debug_TCB *Deubg_TCBCurr = (Debug_TCB *)Debug_RTOS_ThreadListPtr[ThreadID] ; /*get a pointer to the thread stack from the Task Control Block */
		Debug_RegisterWidth *Debug_SP = Deubg_TCBCurr->OSTCBStkPtr;

	if (regno == CPSR_Offset)   /*CPSR*/
		 *RegValue = *(Debug_SP) ;

	else if (regno >= R0_Offset && regno <= R12_Offset )
		*RegValue = *(Debug_SP +(regno+1));
	else if (regno == SP_Offset)
		*RegValue = Debug_SP;
	else if(regno == LR_Offset || regno == PC_Offset)
		*RegValue = *(Debug_SP + regno);

	/*Floating point register IDs are ignored and they are always 0's*/

	    /*
		*********************************************************************************************************
		*                                            2nd Method to read one register
		*********************************************************************************************************
		*/

	/*just read all registers and return the needed one*/

	/*Debug_HAL_Regs_Readall(ThreadID);
	*RegValue = Debug_HAL_RegsBuffer[regno];
*/
	return DEBUG_SUCCESS;

}

/*
*********************************************************************************************************
*                                                Debug_HAL_Regs_WriteOne()
*
* Description :Write one register to Thread register context
*
* Argument(s) : ThreadID : thread id for which this function set register regno.
*               regno    : Register ID as specified by GDB in <P> command packet.
*               RegValue : The register value to be written to thread register regno.
*
* Return(s)   : DEBUG_SUCCESS : if no error
*               DEBUG_ERR_INVALID_REGID if register ID "regno" is not valid.
*
* Caller(s)   :
*
* Note(s)     : (1)Debug_Main_Write_Register() in Debug_Main module
*
*               (2)
*********************************************************************************************************
*/



CPU_INT08U Debug_HAL_Regs_WriteOne(Debug_TID_t ThreadID, Debug_RegID_t regno, Debug_RegisterWidth RegValue)
{

	if(regno > Debug_HAL_Regs_IDMAX)
			return DEBUG_ERR_INVALID_REGID;

	/***************Note : Debug_SP is incremented or decremented based on stack-growth setting"OS_STK_GROWTH macro" in os_cpu.h *********************/

	/*
	*********************************************************************************************************
	*                                             1st Method to write one register
	*********************************************************************************************************
	*/


	Debug_TCB *Deubg_TCBCurr = (Debug_TCB *)Debug_RTOS_ThreadListPtr[ThreadID] ; /*get a pointer to the thread stack from the Task Control Block */
	Debug_RegisterWidth *Debug_SP = Deubg_TCBCurr->OSTCBStkPtr;
	if (regno == CPSR_Offset)   /*CPSR*/
		*(Debug_SP) = RegValue;

	else if (regno >= R0_Offset && regno <= R12_Offset )
		*(Debug_SP +(regno+1)) = RegValue;
	else if (regno == SP_Offset)
		Deubg_TCBCurr->OSTCBStkPtr = RegValue;
	else if(regno == LR_Offset || regno == PC_Offset)
		*(Debug_SP + regno) = RegValue;

	/*Floating point register IDs are ignored and they are always 0's*/


	/*
	*********************************************************************************************************
	*                                             2nd Method to write one register
	*                                               read-modify-write sequence
	*********************************************************************************************************
	*/


	/*Read all registers*/
/*
	 Debug_HAL_Regs_Readall(ThreadID);
	Modify the required register in register buffer
	 Debug_HAL_RegsBuffer[regno] = RegValue ;
	 Write back the register buffer
	 Debug_HAL_Regs_Writeall(ThreadID) ;
*/

	return DEBUG_SUCCESS;

	}

/*
*********************************************************************************************************
*                                                Debug_HAL_Get_StopSignal()
*
* Description :

*
* Argument(s) : ThreadID :
*               signo    :
*
*
* Return(s)   :
*
* Caller(s)   :  Debug_Main_Report_Halt_Reason() in Debug_Main Module
*
* Note(s)     : (1)
*
*               (2)
*********************************************************************************************************
*/
void Debug_HAL_Get_StopSignal(Debug_TID_t ThreadID,Debug_Signal_t *signo)
{
	 /*For this implementation , we only support one signal which is a breakpoint signal SIGTRAP*/
	*signo = SIGTRAP;

}

/*
*********************************************************************************************************
*                                                Debug_HAL_INST_Is_Condition_True()
*
* Description :

*
* Argument(s) : Instruction :
*
*
*
* Return(s)   :
*
* Caller(s)   :  Debug_Main_Step_machine_instruction() in Debug_Main Module
*
* Note(s)     : (1)
*
*               (2)
*********************************************************************************************************
*/

CPU_INT08U Debug_HAL_INST_Is_Condition_True(Debug_MemWidth Instruction,Debug_TID_t ThreadID)
{
	Debug_RegisterWidth Thread_CPSR;
	Debug_MemWidth condition = Instruction & Debug_HAL_INST_COND_BM ;

	/*Get CPSR of ThreadID context*/
		Debug_HAL_Regs_ReadOne(ThreadID,CPSR_Offset,&Thread_CPSR);
switch (condition)
{
        case INST_COND_EQ:
        	if (Thread_CPSR & CPSR_Z_BM )
        		return 1;
        	break;

        case INST_COND_NE:

        	if (!(Thread_CPSR & CPSR_Z_BM )  )
        		return 1;
        	break;

        case INST_COND_CS:
           if (Thread_CPSR & CPSR_C_BM )
        		return 1;
        	break;

        case INST_COND_CC:
        	if (!(Thread_CPSR & CPSR_C_BM )  )
        		return 1;
        	break;


        case INST_COND_MI:
        	if (Thread_CPSR & CPSR_N_BM )
        		return 1;
        	break;

        case INST_COND_PL:
        	if (!(Thread_CPSR & CPSR_N_BM )  )
        		return 1;
        	break;

        case INST_COND_VS:
        	if (Thread_CPSR & CPSR_V_BM )
        		return 1;
        	break;

        case INST_COND_VC:
        	if (!(Thread_CPSR & CPSR_V_BM )  )
        		return 1;
        	break;

        case INST_COND_HI:
        	if ((Thread_CPSR & CPSR_C_BM )  && !(Thread_CPSR & CPSR_Z_BM )  )
        		return 1;
        	break;

        case INST_COND_LS:
        	if ( !(Thread_CPSR & CPSR_C_BM )  || (Thread_CPSR & CPSR_Z_BM )  )
        		return 1;
        	break;

        case  INST_COND_GE:
        	if ( ((Thread_CPSR & CPSR_N_BM ) >> CPSR_N_BP )  == ( (Thread_CPSR & CPSR_V_BM ) >> CPSR_V_BP) )
        		return 1;
        	break;

        case INST_COND_LT:
        	if ( ((Thread_CPSR & CPSR_N_BM ) >> CPSR_N_BP )  != ( (Thread_CPSR & CPSR_V_BM ) >> CPSR_V_BP) )
        		return 1;
        	break;

        case INST_COND_GT:
        	if ( (((Thread_CPSR & CPSR_N_BM ) >> CPSR_N_BP )  == ( (Thread_CPSR & CPSR_V_BM ) >> CPSR_V_BP)  ) && !(Thread_CPSR & CPSR_Z_BM ))
        		return 1;
        	break;

        case INST_COND_LE:
        	if ( ( ((Thread_CPSR & CPSR_N_BM ) >> CPSR_N_BP )  != ( (Thread_CPSR & CPSR_V_BM ) >> CPSR_V_BP)  ) || (Thread_CPSR & CPSR_Z_BM ))
        		return 1;

        	break;

        case INST_COND_AL:
        		return 1;
        	break;
        default :  /* 0x1111 Unconditional_Instruction*/
        	//TODO::
        	return Debug_Instruction_UNCOND; /*to be parsed as unconditional instructions group*/
        	break;
}




	return 0;

}

/*
*********************************************************************************************************
*                                                Debug_HAL_INST_Get_Target_Address()
*
* Description :

*
* Argument(s) : Instruction :
*
*
*
* Return(s)   :
*
* Caller(s)   :  Debug_Main_Step_machine_instruction() in Debug_Main Module
*
* Note(s)     : (1)
*
*               (2)
*********************************************************************************************************
*/

Debug_MemWidth *Debug_HAL_INST_Get_Target_Address(Debug_MemWidth Instruction)
{

	Debug_MemWidth Target_Address;
	/*get op1 value*/
	CPU_INT08U Inst_Class = (Instruction & Debug_HAL_INST_CLASS_BM) >> Debug_HAL_INST_CLASS_BP;

	switch (Inst_Class)
	{
	case INST_CLASS_DataProcess:
		Target_Address = Get_Target_DP_Class(Instruction);
		break;
	case INST_CLASS_LD_STR:
		Target_Address = Get_Target_LDSTR_Class(Instruction);
			break;
	case INST_CLASS_Branch:
		Target_Address = Get_Target_Branch_Class(Instruction);
			break;
	case INST_CLASS_COP_SVC:
		Target_Address = Get_Target_COPSVC_Class(Instruction);
			break;


	}

	return Target_Address;
}

/*
*********************************************************************************************************
*                                                Debug_HAL_UNCONDINST_Get_Target_Address()
*
* Description :

*
* Argument(s) : Instruction :
*
*
*
* Return(s)   :
*
* Caller(s)   :  Debug_Main_Step_machine_instruction() in Debug_Main Module
*
* Note(s)     : (1)
*
*               (2)
*********************************************************************************************************
*/

static Debug_MemWidth Get_Target_DP_Class(Debug_MemWidth Instruction)
{

	/*get Rm operand shared between instructions*/

	Debug_MemWidth Rm = Debug_HAL_RegsBuffer[Instruction & Instruction_Rm_BM];
	/*TODO:: Do we need to check if Rm is PC ,too*/
	Debug_MemWidth Rn ;
	Debug_MemWidth shift_count;
	Debug_MemWidth Target_address;


	if( ((Instruction & DP_Registers_BM) == DP_Registers) || ((Instruction & DP_RegShiftedReg_BM) == DP_RegShiftedReg) || ((Instruction & DP_Immediate_BM) == DP_Immediate)  )
	{
		if( ( (Instruction & PC_RegSpec2_BM) >> PC_RegSpec2_BP ) == Debug_HAL_INST_PC_ID)
		{

			CPU_INT08U imm5Shift = 0;
			if( (Instruction & Instruction_ShiftType_BM) >>Instruction_ShiftType_BP == Shift_LSR || (Instruction & Instruction_ShiftType_BM) >>Instruction_ShiftType_BP == Shift_ASR)
			{
				if ((Instruction & Instruction_imm5_BM) >> Instruction_imm5_BP == 0 )
					imm5Shift = 32 ;
				else
					imm5Shift = (CPU_INT32U)(Instruction & Instruction_imm5_BM) >> Instruction_imm5_BP;

			}
			shift_count = ( !(Instruction & 0x00000010) ) * ( !(Instruction & 0x02000000) ) * (CPU_INT32U)imm5Shift /*DP_Registers*/
			     			   + ( (Instruction & 0x00000010 ) ) * ( !(Instruction & 0x02000000) ) * (CPU_INT32U)(Debug_HAL_RegsBuffer[Instruction & Instruction_Rs_BM])/*DP_RegShiftedReg*/
			     			   +  ( (Instruction & 0x02000000)  * (CPU_INT32U)(Instruction & Instruction_imm12_BM)) ;/*DP_Immediate*/

			/*Check instruction Groups*/
			if((Instruction & DP_Registers_XOperands_BM) >> DP_Registers_XOperands_BP != 0x0D )
			{
				/* only one instruction  here, take two_operand(Rm,immediate) --> MVN(register)*/
               if((Instruction & DP_Registers_XOperands_BM) == DP_Registers_XOperands_BM )
               {

            	   return ~(Shift_Shift_C(Rm,(Instruction & Instruction_ShiftType_BM) >>Instruction_ShiftType_BP ,shift_count));
               }

               else // instruction group that take 3 operand (Rn,Rm,optional_Immediate)
               {
            	   /*Exception : TST,CMP Instruction not affecting PC*/
            	   if( (Instruction & DP_Registers_TST_CMP_BM) == 0x01100000 )
            		   return 0;
            	   /*Get Rn value*/
            	   Rn =  Debug_HAL_RegsBuffer[(Instruction & Instruction_Rn_BM) >>Instruction_Rn_BP];

            	   switch((Instruction & 0x01E00000)>> 21)//op
            	   {
            	      case 0x0://AND
        	    	  return Rn & Shift_Shift_C(Rm,(Instruction & Instruction_ShiftType_BM) >>Instruction_ShiftType_BP , shift_count);
            	    	  break;
            	      case 0x1://EOR
            	    	  return Rn ^ Shift_Shift_C(Rm,(Instruction & Instruction_ShiftType_BM) >>Instruction_ShiftType_BP , shift_count);
            	    	  break;

            	      case 0x2://SUB
            	          if ((Instruction & Instruction_Rn_BM) == Debug_HAL_INST_PC_ID) //ADR-Encoding A2
            	          {
            	        	  /*TODO::should PC here be prefetch i.e. PC= PC+8*/
                             return ((Debug_HAL_RegsBuffer[PC_Offset] >> 2) << 2) - (Debug_MemWidth)(Instruction & Instruction_imm12_BM);
            	          }
            	          else
            	        	  return Rn - Shift_Shift_C(Rm,(Instruction & Instruction_ShiftType_BM) >>Instruction_ShiftType_BP , shift_count);
            	            break;
            	      case 0x3://RSB
						  return  Shift_Shift_C(Rm,(Instruction & Instruction_ShiftType_BM) >>Instruction_ShiftType_BP , shift_count) - Rn;
							break;
            	      case 0x4://ADD
            	    	  if ((Instruction & Instruction_Rn_BM) == Debug_HAL_INST_PC_ID) //ADR-Encoding A1
							  {
            	    		  /*TODO::should PC here be prefetch i.e. PC= PC+8*/
            	    	         return ((Debug_HAL_RegsBuffer[PC_Offset] >> 2) << 2) + (Debug_MemWidth)(Instruction & Instruction_imm12_BM);
							  }
						  else
							  return  Rn + Shift_Shift_C(Rm,(Instruction & Instruction_ShiftType_BM) >>Instruction_ShiftType_BP , shift_count) ;
            	      		break;
            	      case 0x5://ADC
						 return  Rn + (Debug_HAL_RegsBuffer[CPSR_Offset] & 0x20000000)+ Shift_Shift_C(Rm,(Instruction & Instruction_ShiftType_BM) >>Instruction_ShiftType_BP , shift_count) ;
							break;
            	      case 0x6://SBC
						 return  Rn + (Debug_HAL_RegsBuffer[CPSR_Offset] & 0x20000000)+ ~(Shift_Shift_C(Rm,(Instruction & Instruction_ShiftType_BM) >>Instruction_ShiftType_BP , shift_count)) ;
							break;
            	      case 0x7://RSC
						 return  Rn + (Debug_HAL_RegsBuffer[CPSR_Offset] & 0x20000000)+ ~(Shift_Shift_C(Rm,(Instruction & Instruction_ShiftType_BM) >>Instruction_ShiftType_BP , shift_count)) ;
							break;
            	      case 0xC://ORR
						 return  Rn | Shift_Shift_C(Rm,(Instruction & Instruction_ShiftType_BM) >>Instruction_ShiftType_BP , shift_count) ;
							break;
            	      case 0xF://BIC
						 return Rn & (~(Shift_Shift_C(Rm,(Instruction & Instruction_ShiftType_BM) >>Instruction_ShiftType_BP , shift_count)));
							break;

            	   }

               }


			}

			/*Handle instructions with op[21-24] = 1101*/

			/*Extract instruction group that take one operand (MOV(rgister,ARM) ,RRX*/
			else if( ( (Instruction & DP_Registers_1_Operands_BM) == 0x01E00000) ||  ( (Instruction & DP_Registers_1_Operands_BM) == 0x1E00060) )
			{
				/*special case*/
				if (Instruction & 0x00000060) //RRX
				{
					return Shift_Shift_C(Rm,Shift_RRX,1);
				}
                 /*Rm is the target address*/
				return Rm;
			}

			else  /*Rest of instruction that take 2 argument(Rm,Immediate)*/
			{
				/*LSL,LSR,ASR,ROR*/
				return Shift_Shift_C(Rm,(Instruction & Instruction_ShiftType_BM) >>Instruction_ShiftType_BP , shift_count);
			}
		}
		//PC is not Destination Register
	}//1st Sub Class, DP_Registers handling
	//else if((Instruction & DP_RegShiftedReg_BM) == DP_RegShiftedReg)
	//{}
	//else if((Instruction & DP_Immediate_BM) == DP_Immediate)
	//{}
	else if((Instruction & DP_MUL_ACC_BM) == DP_MUL_ACC)
	{}
	else if((Instruction & DP_Satu_ADDSUB_BM) == DP_Satu_ADDSUB)
	{}
	else if((Instruction & DP_HW_MUL_ACC_BM) == DP_HW_MUL_ACC)
	{}
	else if((Instruction & DP_EXTRA_LDSTR_BM) == DP_EXTRA_LDSTR)
	{}
	else if((Instruction & DP_EXTRA_LDSTR_UN_BM) == DP_EXTRA_LDSTR_UN)
	{}
	else if((Instruction & DP_SYNC_BM) == DP_SYNC)
	{}
	else if((Instruction & DP_MSR_Immediate_BM) == DP_MSR_Immediate)
	{}
	else if((Instruction & DP_MISC_BM) == DP_MISC)
	{
		switch (Instruction & 0x00600070)/*Mask op and op2 in Misc subclass*/
		{
			case 0x00200010:  //BX
			case 0x00200020: //BLX(Register)
				return Rm;
				break;
			case 0x00600060://ERET
				/*ALthough this instruction should not be in application code but it'll be weakly handled here,*/
				/*target address is in LR register*/
				return Debug_HAL_RegsBuffer[LR_Offset];
				break;
			case 0x00200070: //BKPT
				/*Breakpoint causes a Prefetch Abort Exception */
				return Excep_Vector_Base + Excep_Vector_Prefetch_Offset;
				break;
		}

	}
	else /*Should it be here undefined instruction*/
		{
		//TODO:: handle odd cases , just return 0 not BAD instruction
			//return DEBUG_Bad_Instruction;
			return 0;
		}
return 0;
}

static Debug_MemWidth Get_Target_LDSTR_Class(Debug_MemWidth Instruction)
{
	Debug_MemWidth Rn = Debug_HAL_RegsBuffer[(Instruction & Instruction_Rn_BM) >>Instruction_Rn_BP];
	Debug_MemWidth * address =0;
	Debug_MemWidth  word =0;
	Debug_MemWidth imm5_Shift;
	if(Rn == 0x000F0000)
		Rn += 8;
	if( ( (Instruction & PC_RegSpec2_BM) >> PC_RegSpec2_BP ) == Debug_HAL_INST_PC_ID)
	{
			switch (Instruction & LDSTR_WB_BM)
			{
			case LDSTR_WB_Encode_A0:
				/*
				 * Index Bit - P at 24
				 * 1--> address = Rn
				 * else --> address = Rn -/+ immediate
				 * Add bit -U at 23*/

				address = Rn   /*TODO: Need Correction*/
						  + ( (!(Instruction & 0x00800000)) *(-1) )/*Check U bit*/
						  *(Instruction & 0x01000000) /*check P bit*/
						  * (CPU_INT32U)(Instruction & Instruction_imm12_BM);

				if ( (Instruction & 0x00500000) == 0x00500000)/*if it is byte access*/
						{
							word = (Debug_MemWidth) ( (CPU_INT08U)(*address) );
						}
				else /*if it is word access*/
					word = *address;

				return word;
				break;

			case LDSTR_WB_Encode_A1:
				imm5_Shift = Shift_Shift_C((Instruction & Instruction_Rm_BM) ,((Instruction & Instruction_ShiftType_BM)>> Instruction_ShiftType_BP),((Instruction & Instruction_imm5_BM)>> Instruction_imm5_BP));
				address = Rn
						  + ( (!(Instruction & 0x00800000)) *(-1) )/*Check U bit*/
						  *(Instruction & 0x01000000) /*check P bit*/
						  * (CPU_INT32U)(imm5_Shift);

								if ( (Instruction & 0x00500000) == 0x00500000)/*if it is byte access*/
										{
											word = (Debug_MemWidth) ( (CPU_INT08U)(*address) );
										}
								else /*if it is word access*/
									word = *address;
				break;
			case LDSTR_WB_Media:
				return Get_Target_LDSTR_Media(Instruction);
				break;

			default:
				/*TODO::should it be here undefined instruction*/
					return DEBUG_Bad_Instruction;
				break;
			}
	}
return 0;
}
static Debug_MemWidth Get_Target_Branch_Class(Debug_MemWidth Instruction)
{
	/*Is it Branching(B,BLX(immediate) or LD/POP)*/
	if (Instruction & 0x02000000)
	{
		/*B or BLX(immediate)*/
		/*targetAddress = Align(PC,4) + SignExtend(imm24:'00', 32);*/
		//return (Debug_HAL_RegsBuffer[PC_Offset] & 0x06) + (CPU_INT32S)(Instruction & Instruction_imm24_BM);

		return ((Debug_HAL_RegsBuffer[PC_Offset] >> 2) << 2) + (CPU_INT32S)(Instruction & Instruction_imm24_BM);

	}
	else
	{
		/*LDMxx/POP*/

		if(Instruction & PC_RegList_BM && (Instruction & 0x00100000))/*Is PC in Reg_List and we are in Load/POP not store/push*/
		{

			Debug_MemWidth Rn = Debug_HAL_RegsBuffer[(Instruction &Instruction_Rn_BM) >> Instruction_Rn_BP];
			CPU_INT08U regs_Number_loaded = Count_NumRegsLoaded((CPU_INT16U)(Instruction & 0x0000FFFF)); /*?? to be calculated*/
			Debug_MemWidth *address ;
			/*Are we increment or decrement*/
			if((Instruction & 0x02C00000) == 0x00800000) /*Increment*/
			{
				address = Rn + (4*regs_Number_loaded) - (4*( (Instruction & 0x03400000) == 0x00000000));/*subtract 4 if increment after*/
			}
			else if(!(Instruction & 0x02C00000))/*Decrement*/

			{
				address = Rn - (4* ((Instruction & 0x03400000) == 0x01000000) );/*subtract 4 if increment before*/
			}
	        else /*LDM(Exception Return) TODO::??*/
				{}
			return (*address);

		}

	}

	return 0;

}
static Debug_MemWidth Get_Target_COPSVC_Class(Debug_MemWidth Instruction)
{
	/*is it SVC call*/
	if ( (Instruction & 0x03000000) == 0x03000000 )
	{
		return Excep_Vector_Base + Excep_Vector_SVC_Offset ;/*SVC entry address in Vector table*/
	}
	return 0;
}

static Debug_MemWidth Get_Target_LDSTR_Media(Debug_MemWidth Instruction)
{

	switch (Instruction & LDSTR_WB_Media_ParallelAddSub_BM)
	{
	 	 case LDSTR_WB_Media_ParallelAddSub_Sign:
	 		 break;
	 	 case LDSTR_WB_Media_ParallelAddSub_Unsign:
	 		 break;

	 	 default:/*TODO::Should it be undefined instruction*/
	 		 break;
	}

	switch (Instruction & LDSTR_WB_Media_PackUn_Sat_Rev_MUL_DIV_BM)
	{
		case LDSTR_WB_Media_PackUn_Sat_Rev:
			break;
		case LDSTR_WB_Media_MUL_DIV:
			break;
		default: /*TODO:: should it be undefined instruction*/
			break;

	}
return 0;
}
static Debug_MemWidth Shift_Shift_C(Debug_MemWidth operand, CPU_INT08U shift_t, CPU_INT08U shift_n)
{

	CPU_INT08U Shift_n_moduluo =  shift_n%32;
	if (!shift_n)
		return operand;

	switch (shift_t)
	{
	case Shift_LSL:
		return operand << shift_n;
		break;
	case Shift_LSR:
		return operand >> shift_n;
		break;
	case Shift_ASR:
		return (operand >>shift_n) | (operand & 0x80000000);
		break;
	case Shift_ROR:
		return (operand >> Shift_n_moduluo) | (operand << (32 -Shift_n_moduluo ));
		break;
	case Shift_RRX:
		return (operand >> 1) | ( (Debug_HAL_RegsBuffer[CPSR_Offset] & 0x20000000) << 1) ;
		break;


	}
	return 0;
}
static CPU_INT08U Count_NumRegsLoaded( CPU_INT16U CPU_RegList)
{
	CPU_INT08U count = 0;
	 while(CPU_RegList)
	  {
		count += CPU_RegList & 1;
		CPU_RegList >>= 1;
	  }
	  return count;

}
static Debug_MemWidth Get_Target_UNCOND_Class(Debug_MemWidth Instruction)
{

	Debug_MemWidth Rn = Debug_HAL_RegsBuffer[(Instruction &Instruction_Rn_BM) >> Instruction_Rn_BP];
	Debug_MemWidth *address;
	/*check each instruction*/
	if((Instruction & 0x0E500000) == 0x08100000)  /*Is it RFE */
	{

		/**/
		address = Rn
				                     -( 8 * (!(Instruction & 0x00800000)) )/*subtract 8 if U bit[23] is cleared*/
				                     + (4 * ( (Instruction & 0x00800000) == (Instruction & 0x01000000) ) );/*add4 if P bit[24] == U bit*/
	return (*address);
	}

	if((Instruction & 0x0E000000) == 0x0A000000)/*Is it BL,BLX */
	{

		/*B or BLX(immediate)*/
				/*targetAddress = Align(PC,4) + SignExtend(imm24:'00', 32);*/
				//return (Debug_HAL_RegsBuffer[PC_Offset] & 0x06) + (CPU_INT32S)(Instruction & Instruction_imm24_BM);

				return ((Debug_HAL_RegsBuffer[PC_Offset] >> 2) << 2) + (CPU_INT32S)(Instruction & Instruction_imm24_BM);
	}

	if((Instruction & 0x0FF00000) == 0x0C500000)/*Is it MRRC,MRRC2 */
	{
		/*Is PC is the destination registers ? */
		if( ((Instruction & Instruction_Rt_BM)>>Instruction_Rt_BP) == Debug_HAL_INST_PC_ID)
		{

		}
		else if ( ((Instruction & Instruction_Rt2_BM) >> Instruction_Rt2_BP) == Debug_HAL_INST_PC_ID)
		{

		}

	}

	if((Instruction & 0x0F100010) == 0x0E100010)/*Is it MRC,MRC2 */
	{
			/*not affecting PC*/
	}
	return 0;
}
