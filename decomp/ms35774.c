/*
quick and dirty ghidra output, not compileable as-is, but
should be enough detail for a reimpl (this isn't a complex driver)
*/

void ms35774_driver_init(void)
{
  _mcount();
  __platform_driver_register(&PTR_ms35774_probe_ffffff8009838cd0,0);
  return;
}


undefined8 m35774_run_thread_handler(stepper_state_struct *stepper_state)
{
  int iVar1;
  int iVar2;
  bool bVar3;
  long lVar4;
  ulong uVar5;
  char *pcVar6;
  int iVar7;
  undefined auStack_90 [40];
  long canary;
  
  canary = DAT_ffffff80097adb08;
  _mcount();
  do {
    if (stepper_state->flag_needs_update == '\0') {
      init_wait_entry(auStack_90,0);
      while( true ) {
        lVar4 = prepare_to_wait_event(&STEPPER_WAIT_QUEUE,auStack_90,1);
        if (stepper_state->flag_needs_update != '\0') break;
        if (lVar4 != 0) goto LAB_ffffff80086721a8;
        schedule();
      }
      finish_wait(&STEPPER_WAIT_QUEUE,auStack_90);
    }
LAB_ffffff80086721a8:
    _dev_info(stepper_state->dev,"[%s] run\n","m35774_run_thread_handler");
    if (stepper_state->angle_now == stepper_state->angle_target) {
      stepper_state->flag_needs_update = '\0';
      pcVar6 = "[%s] same orientation\n";
    }
    else {
      if (stepper_state->angle_target < stepper_state->angle_now) {
        pinctrl_select_state(stepper_state->pinctrlr,stepper_state->pin_dir_low);
        iVar1 = stepper_state->angle_now;
        iVar2 = stepper_state->angle_target;
        iVar7 = iVar1 - iVar2;
      }
      else {
        pinctrl_select_state(stepper_state->pinctrlr,stepper_state->pin_dir_high);
        iVar1 = stepper_state->angle_now;
        iVar2 = stepper_state->angle_target;
        iVar7 = iVar2 - iVar1;
      }
      _dev_info(stepper_state->dev,"[%s] o:%d, or:%d, step:%d\n","m35774_run_thread_handler",iVar1,
                iVar2,iVar7 * 6);
      pinctrl_select_state(stepper_state->pinctrlr,stepper_state->pin_vm_high);
      __const_udelay(42950000); // 10000
      pinctrl_select_state(stepper_state->pinctrlr,stepper_state->pin_enn_low);
      if (iVar7 * 6 != 0) {
        iVar7 = iVar7 * -6;
        do {
          pinctrl_select_state(stepper_state->pinctrlr,stepper_state->pin_step_high);
          __const_udelay(214750); // 50
          pinctrl_select_state(stepper_state->pinctrlr,stepper_state->pin_step_low);
          __const_udelay(1546200); // 360
          bVar3 = iVar7 != -1;
          iVar7 = iVar7 + 1;
        } while (bVar3);
      }
      __const_udelay(214750); // 50
      pinctrl_select_state(stepper_state->pinctrlr,stepper_state->pin_enn_high);
      pinctrl_select_state(stepper_state->pinctrlr,stepper_state->pin_vm_low);
      stepper_state->flag_needs_update = '\0';
      stepper_state->angle_now = stepper_state->angle_target;
      pcVar6 = "[%s] done\n";
    }
    _dev_info(stepper_state->dev,pcVar6,"m35774_run_thread_handler");
    uVar5 = kthread_should_stop();
    if ((uVar5 & 1) != 0) {
      if (DAT_ffffff80097adb08 == canary) {
        return 0;
      }
                    /* WARNING: Subroutine does not return */
      __stack_chk_fail();
    }
  } while( true );
}


void ms35774_init_work_handler(stepper_state_struct *param_1)

{
  bool bVar1;
  long lVar2;
  
  _mcount();
  param_1->pin_dir_high = 180;
  *(undefined *)&param_1->pin_dir_low = 1;
  __wake_up(&STEPPER_WAIT_QUEUE,1,1,0);
  lVar2 = -2000;
  do {
    __const_udelay(0x418958); // 1000
    bVar1 = lVar2 != -1;
    lVar2 = lVar2 + 1;
  } while (bVar1);
  *(undefined4 *)((long)&param_1->pin_dir_high + 4) = 90;
  *(undefined *)&param_1->pin_dir_low = 1;
  __wake_up(&STEPPER_WAIT_QUEUE,1,1,0);
  return;
}



int ms35774_probe(long param_1)
{
  int iVar1;
  stepper_state_struct *plVar2;
  ulong uVar2;
  undefined8 uVar3;
  long lVar4;
  
  _mcount();
  lVar4 = param_1 + 0x10;
  plVar2 = (stepper_state_struct *)devm_kmalloc(lVar4,0xc0,0x6080c0); // ___GFP_IO_BIT, ___GFP_FS_BIT, 
  if (plVar2 == (stepper_state_struct *)0x0) {
    iVar1 = -0xc;
  }
  else {
    plVar2->dev = lVar4;
    *(stepper_state_struct **)(param_1 + 200) = plVar2;
    _dev_info(lVar4,"[%s] enter\n","ms35774_probe");
    uVar2 = devm_pinctrl_get(plVar2->dev);
    plVar2->pinctrlr = uVar2;
    if (uVar2 < 0xfffffffffffff001) {
      lVar4 = 0;
      do {
        uVar3 = *(undefined8 *)((long)PIN_NAMES + lVar4);
        uVar2 = pinctrl_lookup_state(uVar2,uVar3);
        *(ulong *)((long)&plVar2->pin_vm_high + lVar4) = uVar2;
        if (0xfffffffffffff000 < uVar2) {
          _dev_err(plVar2->dev,"[%s] fail to get pinctrl %s\n","ms35774_parse_dts",uVar3);
          goto LAB_ffffff8008672488;
        }
        uVar2 = plVar2->pinctrlr;
        lVar4 = lVar4 + 8;
      } while (lVar4 != 0x70);
      pinctrl_select_state(uVar2,plVar2->pin_vm_low);
      pinctrl_select_state(plVar2->pinctrlr,plVar2->pin_enn_high);
      pinctrl_select_state(plVar2->pinctrlr,plVar2->pin_dir_low);
      pinctrl_select_state(plVar2->pinctrlr,plVar2->pin_step_low);
      pinctrl_select_state(plVar2->pinctrlr,plVar2->pin_pdn_low);
      pinctrl_select_state(plVar2->pinctrlr,plVar2->pin_ms1_high);
      pinctrl_select_state(plVar2->pinctrlr,plVar2->pin_ms2_low);
      plVar2->angle_now = 0;
      plVar2->angle_target = 0;
      plVar2->flag_needs_update = '\0';
      uVar2 = kthread_create_on_node
                        (m35774_run_thread_handler,plVar2,0xffffffff,"m35774_step_motor_run_thread")
      ;
      if (uVar2 < 0xfffffffffffff001) {
        wake_up_process(uVar2);
        DAT_ffffff8009ac2a00 = uVar2;
        *(undefined8 *)&plVar2->field_0x80 = 0xfffffffe0;
        *(undefined **)&plVar2->field_0x88 = &plVar2->field_0x88;
        *(undefined **)&plVar2->field_0x90 = &plVar2->field_0x88;
        *(code **)&plVar2->field_0x98 = ms35774_init_work_handler;
        iVar1 = device_create_file(plVar2->dev,&PTR_s_orientation_ffffff8009838db0);
        if (iVar1 == 0) {
          DAT_ffffff8009ac29f8 = plVar2;
          queue_work_on(0x40,DAT_ffffff80097adb70,&plVar2->field_0x80);
          _dev_info(plVar2->dev,"[%s] success!\n","ms35774_probe");
        }
        else {
          _dev_err(plVar2->dev,"[%s] fail to create sysfs attr! ret:%d\n","ms35774_probe",iVar1);
        }
      }
      else {
        DAT_ffffff8009ac2a00 = uVar2;
        _dev_err(plVar2->dev,"[%s] fail to create motor run thread\n","ms35774_probe");
        iVar1 = (int)DAT_ffffff8009ac2a00;
      }
    }
    else {
      _dev_err(plVar2->dev,"[%s] fail to get pinctrl\n","ms35774_parse_dts");
LAB_ffffff8008672488:
      _dev_err(plVar2->dev,"[%s] fail to parse dts\n","ms35774_probe");
      iVar1 = -0x13;
    }
  }
  return iVar1;
}



long show_m35774_orientation(long param_1,undefined8 param_2,char *param_3)

{
  int iVar1;
  
  _mcount();
  iVar1 = sprintf(param_3,"%d\n",(ulong)*(uint *)(*(long *)(param_1 + 0xb8) + 0xb0));
  return (long)iVar1;
}



undefined8 store_m35774_orientation(long param_1,undefined8 param_2,undefined8 buf,undefined8 count)
{
  int iVar1;
  undefined8 uVar2;
  char *pcVar3;
  stepper_state_struct *puVar4;
  uint orientation_int;
  long local_38;
  
  local_38 = DAT_ffffff80097adb08;
  _mcount();
  puVar4 = *(stepper_state_struct **)(param_1 + 0xb8);
  iVar1 = kstrtoint(buf,10,&orientation_int);
  if (iVar1 == 0) {
    if (orientation_int < 181) {
      puVar4->angle_target = orientation_int;
      puVar4->flag_needs_update = '\x01';
      __wake_up(&STEPPER_WAIT_QUEUE,1,1,0);
      goto LAB_ffffff8008672728;
    }
    uVar2 = puVar4->dev;
    pcVar3 = "[%s] wrong request orientation(%d), must be 0~180\n";
  }
  else {
    uVar2 = puVar4->dev;
    pcVar3 = "[%s] format error, request int 0~180\n";
  }
  _dev_err(uVar2,pcVar3,"store_m35774_orientation");
LAB_ffffff8008672728:
  if (DAT_ffffff80097adb08 == local_38) {
    return count;
  }
                    /* WARNING: Subroutine does not return */
  __stack_chk_fail();
}




void ms35774_shutdown(long param_1)
{
  bool bVar1;
  stepper_state_struct *lVar2;
  long lVar3;
  
  _mcount();
  lVar2 = *(stepper_state_struct **)(param_1 + 200);
  lVar2->angle_target = 90;
  lVar2->flag_needs_update = '\x01';
  __wake_up(&STEPPER_WAIT_QUEUE,1,1,0);
  lVar3 = -0x5dc;
  do {
    __const_udelay(4295000);
    bVar1 = lVar3 != -1;
    lVar3 = lVar3 + 1;
  } while (bVar1);
  return;
}
