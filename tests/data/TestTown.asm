	object_const_def
	const_export TESTTOWN_GIRL

TestTown_Object:
	db $b ; border block

	def_warp_events
	warp_event  5,  5, TEST_HOUSE, 1
	warp_event 13,  5, TEST_LAB_2F, 2

	def_bg_events
	bg_event  3,  5, TEXT_TESTTOWN_SIGN

	def_object_events
	object_event  8,  5, SPRITE_GIRL, WALK, ANY_DIR, TEXT_TESTTOWN_GIRL

	def_warps_to TEST_TOWN
