int __cdecl find_device(int ctx)
{
  char v2; // [esp+12h] [ebp-36h]
  __int16 v3; // [esp+1Ah] [ebp-2Eh]
  __int16 v4; // [esp+1Ch] [ebp-2Ch]
  int v5; // [esp+24h] [ebp-24h]
  int v6; // [esp+28h] [ebp-20h]
  int v7; // [esp+2Ch] [ebp-1Ch]
  int v8; // [esp+30h] [ebp-18h]
  int v9; // [esp+34h] [ebp-14h]
  int i; // [esp+38h] [ebp-10h]
  int v11; // [esp+3Ch] [ebp-Ch]

  v11 = 0;
  v9 = libusb_get_device_list(0, &v6);
  i = 0;
  v8 = 0;
  if ( v9 < 0 )
    perror("Error getting list of devices");
  for ( i = 0; i < v9; ++i )
  {
    v7 = *(_DWORD *)(4 * i + v6);
    libusb_get_device_descriptor(v7, &v2);
    if ( v3 == 0x955 && v4 == 0x7820 )
    {
      puts("Found APX mode device");
      v11 = v7;
      break;
    }
  }
  v5 = 0;
  if ( v11 )
  {
    v8 = libusb_open(v11, &v5);
    if ( v8 )
      fwrite("Error opening device\n", 1u, 0x15u, stderr);
  }
  libusb_free_device_list(v6, 1);
  return v5;
}

void __cdecl getEndpoints(int a1, _BYTE *a2, _BYTE *a3)
{
  signed int v3; // [esp+8h] [ebp-8h]
  signed int i; // [esp+Ch] [ebp-4h]

  v3 = -2;
  for ( i = 0; *(unsigned __int8 *)(**(_DWORD **)(a1 + 12) + 4) > i; ++i )
  {
    if ( *(_BYTE *)(*(_DWORD *)(**(_DWORD **)(a1 + 12) + 12) + 20 * i + 2) >= 0 )
      *a3 = *(_BYTE *)(*(_DWORD *)(**(_DWORD **)(a1 + 12) + 12) + 20 * i + 2);
    else
      *a2 = *(_BYTE *)(*(_DWORD *)(**(_DWORD **)(a1 + 12) + 12) + 20 * i + 2);
    ++v3;
  }
}

int __cdecl main(int argc, const char **argv, const char **envp)
{
  int v4; // eax
  __int16 desc; // [esp+2Eh] [ebp-42h]
  __int16 v6; // [esp+3Ah] [ebp-36h]
  int data1; // [esp+40h] [ebp-30h]
  int v8; // [esp+44h] [ebp-2Ch]
  int data2; // [esp+4Ch] [ebp-24h]
  int transfered2; // [esp+50h] [ebp-20h]
  int transfered1; // [esp+54h] [ebp-1Ch]
  int config; // [esp+58h] [ebp-18h]
  int ctx; // [esp+5Ch] [ebp-14h]
  int device; // [esp+60h] [ebp-10h]
  int handle; // [esp+64h] [ebp-Ch]
  int ret; // [esp+68h] [ebp-8h]
  char ep_out; // [esp+6Eh] [ebp-2h]
  char ep_in; // [esp+6Fh] [ebp-1h]

  ret = 0;
  handle = 0;
  config = 0;
  device = 0;
  ret = libusb_init((int)&ctx);
  libusb_set_debug(ctx, 3);
  handle = find_device(ctx);
  device = libusb_get_device(handle);
  if ( libusb_get_active_config_descriptor(device, &config) )
  {
    fwrite("Error getting active configuration descriptor\n", 1u, 0x2Eu, stderr);
    return -1;
  }
  libusb_get_device_descriptor(device, &desc);
  if ( v6 == 0x104 )
  {
    if ( libusb_claim_interface(handle, 0) )
    {
      fwrite("Error claiming interface\n", 1u, 0x19u, stderr);
      return -1;
    }
    getEndpoints(config, &ep_in, &ep_out);
    if ( v4 )
    {
      fwrite("Error retrieving endpoints\n", 1u, 0x1Bu, stderr);
      return -1;
    }
    transfered1 = 0;
    if ( libusb_bulk_transfer(handle, (unsigned __int8)ep_in, &data1, 8, &transfered1, 0) )
    {
      fwrite("Error in bulk transfer\n", 1u, 0x17u, stderr);
      return -1;
    }
    printf("Chip UID: 0x%llx\n", data1, v8);
    if ( libusb_bulk_transfer(handle, (unsigned __int8)ep_out, getversioncmd, 1028, &transfered2, 0) )
    {
      fwrite("Error in bulk transfer\n", 1u, 0x17u, stderr);
      return -1;
    }
    data2 = 0;
    if ( libusb_bulk_transfer(handle, (unsigned __int8)ep_in, &data2, 4, &transfered1, 0) )
    {
      fwrite("Error in bulk transfer\n", 1u, 0x17u, stderr);
      return -1;
    }
    libusb_release_interface(handle, 0);
    if ( data2 == 0x20001 )
      puts("Detected SBKv2");
    else
      puts("Detected SBKv1");
  }
  else
  {
    puts("Detected SBKv1");
  }
  libusb_free_config_descriptor(config);
  libusb_close(handle);
  libusb_exit(ctx);
  return 0;
}
