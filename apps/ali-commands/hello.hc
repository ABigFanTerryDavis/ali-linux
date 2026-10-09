// hello.hc - first HolyC on ALI (hcc v1 subset)
U0 Main()
{
  Print("Hello from HolyC on ALI Linux.\n");
  I64 i;
  for (i = 0; i < 3; i = i + 1)
    Print("roll %lld: the dice remembers Terry.\n", i + 1);
}
