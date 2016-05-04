find_package(X11)

set(UG_ENABLE_SYSTEM_HEAP True CACHE BOOL
  "Whether to use the UG heap or the one of the operating system")

# Compatibility against previous UG versions.
set(UG_USE_SYSTEM_HEAP ${UG_ENABLE_SYSTEM_HEAP})
set(UG_FOUND True)
set(HAVE_UG True)
set(UG_VERSION "${DUNE_UGGRID_VERSION}")

if(MPI_C_FOUND)
  # Actually we probably should activate UG
  # for everything. But for the being we fall
  # back to the enable trick. To change this just
  # uncomment the lines below.
  #add_definitions("-DENABLE_UG=1")
  #add_definitions("-DModelP")
  set(UG_DEFINITIONS "ENABLE_UG=1;ModelP")
  set(UG_PARALLEL "yes")
else()
  # Actually we probably should activate UG
  # for everything. But for the being we fall
  # back to the enable trick. To change this just
  # uncomment the lines below.
  #add_definitions("-DENABLE_UG=1")
  set(UG_DEFINITIONS "ENABLE_UG=1")
  set(UG_PARALLEL "no")
endif()

message("UG_USE_SYSTEM_HEAP=${UG_USE_SYSTEM_HEAP}")
message("UG_ENABLE_SYSTEM_HEAP=${UG_ENABLE_SYSTEM_HEAP}|")
