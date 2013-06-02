 ; author: r57shell
 ; This is file, HOW MUST BE USED GEMS_Player.bin
 org $0
 incbin GEMS_Player.bin

 ; callbacks RIGHT after GEMS_Player.bin rom
 ; you can use ANY driver, if you will make correct callbacks.
 jmp gemsinit       ; driver init
 jmp gemsstartsong  ; start song: called when start play BGM or SFX
 jmp gemsstopsong   ; stop song: called when start play BGM.
 jmp gemsprogchange ; change patch: used to play DAC
 jmp gemsnoteon     ; turn note on: used to play DAC
 jmp gemsstopall    ; stop all sequences
 jmp gemsresumeall  ; resume all sequences
 jmp gemspauseall   ; pause all sequences
 jmp gemsdmastart   ; called before DMA
 jmp gemsdmaend     ; called after DMA
 jmp gemsmute       ; mute/unmute channel
 jmp gemsmasteratn  ; set master attenuation (used with fade)
 jmp gemssettempo   ; set tempo
 jmp gemsholdz80    ; hold z80 bus
 jmp gemsreleasez80 ; release z80 bus
 jmp custom_update  ; your custom update
 ; RIGHT after them
 dc.l $A00884+1 ; tempo ptr, set to 0 if you don't know.
 dc.w _sequences_count-1 ; sequences count (BGM = SFX)
 dc.b _samples_count-1 ; dac count
 dc.b _dac_patch ; dac patch
 
 even
 
 include 'sequences_info.asm'

 include 'GEMS.asm'
 
custom_update:
 ; do your stuff, if you know how
 rts
