OldTown_Object:
	db $b ; border block

	db 2 ; warps
	warp 5, 5, 0, OLD_HOUSE
	warp 13, 5, 1, OLD_LAB

	db 1 ; signs
	sign 3, 5, 2 ; OldTownSign

	db 1 ; objects
	object SPRITE_GIRL, 8, 5, WALK, 0, 1 ; person

	; warp-to
	warp_to 5, 5, OLD_TOWN_WIDTH
	warp_to 13, 5, OLD_TOWN_WIDTH
