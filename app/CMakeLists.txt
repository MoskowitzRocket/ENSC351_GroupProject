# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(plant_waterer ${MY_SOURCES})

# Make use of the HAL library
target_link_libraries(plant_waterer LINK_PRIVATE hal LINK_PUBLIC gpiod)

# Copy executable to final location (change `plant_waterer` to project name as needed)
add_custom_command(TARGET plant_waterer POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:plant_waterer>"
     "~/ENSC351/public/myApps/plant_waterer" 
  COMMENT "Copying ARM executable to public NFS directory")



  