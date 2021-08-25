execute_process(COMMAND git submodule init			
				WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
execute_process(COMMAND git submodule update			
				WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
execute_process(COMMAND git submodule foreach --recursive git pull origin master		
				WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})