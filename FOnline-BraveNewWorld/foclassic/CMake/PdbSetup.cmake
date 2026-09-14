## Prepare .pdb files in all VS configurations, including Release
function( PdbSetup target )
    if( MSVC )
        set( pdb "${target}.pdb" )

        set_property( TARGET ${target} APPEND_STRING PROPERTY COMPILE_FLAGS "/Zi " )
        set_property( TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS "/DEBUG /PDBALTPATH:${pdb} " )
    endif()
endfunction()