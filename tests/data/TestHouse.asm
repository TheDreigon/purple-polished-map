	object_const_def
	const TESTHOUSE_CLERK

TestHouse_MapScripts:
	def_scene_scripts

	def_callbacks

TestHouseClerkScript:
	jumptextfaceplayer TestHouseClerkText

TestHouseLadderScript:
	warp TEST_CAVE, 3, 4
	end

TestHouseClerkText:
	text "Hello!"
	done

TestHouse_MapEvents:
	db 0, 0 ; filler

	def_warp_events
	warp_event  2,  7, TEST_TOWN, 1
	warp_event  3,  7, TEST_TOWN, 1
	warp_event  9,  0, TEST_HOUSE, 1

	def_coord_events
	coord_event  5,  4, SCENE_TESTHOUSE_NOOP, TestHouseLadderScript

	def_bg_events
	bg_event  0,  1, BGEVENT_READ, TestHouseClerkScript
	bg_event $a, $1, BGEVENT_READ, TestHouseClerkScript ; hex coordinates

	def_object_events
	object_event  7,  4, SPRITE_CLERK, SPRITEMOVEDATA_STANDING_LEFT, 0, 0, -1, -1, 0, OBJECTTYPE_SCRIPT, 0, TestHouseClerkScript, -1
