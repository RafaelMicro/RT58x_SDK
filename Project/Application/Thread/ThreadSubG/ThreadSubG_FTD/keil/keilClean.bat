del *.bak /s
del *.ddk /s
del *.edk /s
del *.lst /s
del *.lnp /s
del *.mpf /s
del *.mpj /s
del *.obj /s
del *.Output/Release_Lib /s
del *.omf /s
del *.hex /s
del *.h /s
::del *.bin /s
del *.elf /s
::del *.opt /s  ::不允许删除JLINK的设置
del *.plg /s
del *.rpt /s
del *.tmp /s
del *.__i /s
del *.crf /s
del *.o /s
del *.d /s
del *.axf /s
del *.tra /s
del *.dep /s           
del JLinkLog.txt /s

RD /S /Q "Listings"
RD /S /Q "Objects"
RD /S /Q "RTE"
del *.iex /s
del *.P350 /s
del *.htm /s
del *.map /s
del *.dell /s
exit
