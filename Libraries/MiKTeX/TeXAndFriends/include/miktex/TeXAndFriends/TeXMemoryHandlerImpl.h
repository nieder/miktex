/* miktex/TeXAndFriends/TeXMemoryHandlerImpl.h:         -*- C++ -*-

   Copyright (C) 2017 Christian Schenk

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 2, or (at your
   option) any later version.

   This file is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this file; if not, write to the Free Software
   Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA. */

#if defined(_MSC_VER)
#  pragma once
#endif

#if !defined(F751C3C43D1A411F806871C3A791B48B)
#define F751C3C43D1A411F806871C3A791B48B

#include <miktex/TeXAndFriends/config.h>

#include "TeXMFMemoryHandlerImpl.h"

MIKTEXMF_BEGIN_NAMESPACE;

template<class PROGRAM_CLASS> class TeXMemoryHandlerImpl :
  public TeXMFMemoryHandlerImpl<PROGRAM_CLASS>
{
public:
  TeXMemoryHandlerImpl(PROGRAM_CLASS& program, TeXMFApp& texmfapp) :
    TeXMFMemoryHandlerImpl(program, texmfapp)
  {
  }

public:
  void Allocate(const std::unordered_map<std::string, int>& userParams) override
  {
    TeXMFMemoryHandlerImpl::Allocate(userParams);
    GETPARAM(param_mem_bot, membot, mem_bot, 0);

    if (THEDATA(membot) < 0 || THEDATA(membot) > 1)
    {
      MIKTEX_FATAL_ERROR(MIKTEXTEXT("mem_bot must be 0 or 1."));
    }

    TeXMFApp::AllocateMemory();

    GETPARAMCHECK(param_max_in_open, maxinopen, max_in_open, texapp::texapp::max_in_open());
    GETPARAMCHECK(param_nest_size, nestsize, nest_size, texapp::texapp::nest_size());
    GETPARAMCHECK(param_save_size, savesize, save_size, texapp::texapp::save_size());
    GETPARAM(param_trie_size, triesize, trie_size, texapp::texapp::trie_size());

#if ! defined(MIKTEX_OMEGA)
    GETPARAM(param_hash_extra, hashextra, hash_extra, texapp::texapp::hash_extra());
#endif

    Allocate("sourcefilenamestack", THEDATA(sourcefilenamestack), THEDATA(maxinopen));
    Allocate("linestack", THEDATA(linestack), THEDATA(maxinopen));
    Allocate("fullsourcefilenamestack", THEDATA(fullsourcefilenamestack), THEDATA(maxinopen));
    Allocate("inputfile", THEDATA(inputfile), THEDATA(maxinopen));
    Allocate("nest", THEDATA(nest), THEDATA(nestsize) + 1);
    Allocate("savestack", THEDATA(savestack), THEDATA(savesize) + 1);
    Allocate("triehash", THEDATA(triehash), THEDATA(triesize) + 1);
    Allocate("triel", THEDATA(triel), THEDATA(triesize) + 1);
    Allocate("trieo", THEDATA(trieo), THEDATA(triesize) + 1);
    Allocate("trier", THEDATA(trier), THEDATA(triesize) + 1);
    Allocate("trietaken", THEDATA(trietaken), THEDATA(triesize));

    Allocate("nameoffile", THEDATA(nameoffile), MiKTeX::Core::BufferSizes::MaxPath + 1);

#if ! defined(MIKTEX_OMEGA)
    GETPARAMCHECK(param_hyph_size, hyphsize, hyph_size, texapp::texapp::hyph_size());
    GETPARAM(param_font_max, fontmax, font_max, texapp::texapp::font_max());
    GETPARAMCHECK(param_font_mem_size, fontmemsize, font_mem_size, texapp::texapp::font_mem_size());

    Allocate("trietrl", THEDATA(trietrl), THEDATA(triesize));
    Allocate("trietro", THEDATA(trietro), THEDATA(triesize));
    Allocate("trietrc", THEDATA(trietrc), THEDATA(triesize));

    Allocate("hyphword", THEDATA(hyphword), THEDATA(hyphsize));
    Allocate("hyphlist", THEDATA(hyphlist), THEDATA(hyphsize));
    Allocate("hyphlink", THEDATA(hyphlink), THEDATA(hyphsize));

    Allocate("bcharlabel", THEDATA(bcharlabel), THEDATA(fontmax) - constfontbase);
    Allocate("charbase", THEDATA(charbase), THEDATA(fontmax) - constfontbase);
    Allocate("depthbase", THEDATA(depthbase), THEDATA(fontmax) - constfontbase);
    Allocate("extenbase", THEDATA(extenbase), THEDATA(fontmax) - constfontbase);
    Allocate("fontarea", THEDATA(fontarea), THEDATA(fontmax) - constfontbase);
    Allocate("fontbc", THEDATA(fontbc), THEDATA(fontmax) - constfontbase);
    Allocate("fontbchar", THEDATA(fontbchar), THEDATA(fontmax) - constfontbase);
    Allocate("fontcheck", THEDATA(fontcheck), THEDATA(fontmax) - constfontbase);
    Allocate("fontdsize", THEDATA(fontdsize), THEDATA(fontmax) - constfontbase);
    Allocate("fontec", THEDATA(fontec), THEDATA(fontmax) - constfontbase);
    Allocate("fontfalsebchar", THEDATA(fontfalsebchar), THEDATA(fontmax) - constfontbase);
    Allocate("fontglue", THEDATA(fontglue), THEDATA(fontmax) - constfontbase);
    Allocate("fontname", THEDATA(fontname), THEDATA(fontmax) - constfontbase);
    Allocate("fontparams", THEDATA(fontparams), THEDATA(fontmax) - constfontbase);
    Allocate("fontsize", THEDATA(fontsize), THEDATA(fontmax) - constfontbase);
    Allocate("fontused", THEDATA(fontused), THEDATA(fontmax) - constfontbase);
    Allocate("heightbase", THEDATA(heightbase), THEDATA(fontmax) - constfontbase);
    Allocate("hyphenchar", THEDATA(hyphenchar), THEDATA(fontmax) - constfontbase);
    Allocate("italicbase", THEDATA(italicbase), THEDATA(fontmax) - constfontbase);
    Allocate("kernbase", THEDATA(kernbase), THEDATA(fontmax) - constfontbase);
    Allocate("ligkernbase", THEDATA(ligkernbase), THEDATA(fontmax) - constfontbase);
    Allocate("parambase", THEDATA(parambase), THEDATA(fontmax) - constfontbase);
    Allocate("skewchar", THEDATA(skewchar), THEDATA(fontmax) - constfontbase);
    Allocate("triec", THEDATA(triec), THEDATA(triesize));
    Allocate("widthbase", THEDATA(widthbase), THEDATA(fontmax) - constfontbase);

    if (IsInitProgram())
    {
      // memory allocated in tex-miktex-hash.ch
      THEDATA(yhash) = 0;
      THEDATA(zeqtb) = 0;
    }

    if (IsInitProgram() || !AmITeXCompiler() || AmI("omega"))
    {
      Allocate("fontinfo", THEDATA(fontinfo), THEDATA(fontmemsize));
    }
#endif // not Omega

#if defined(MIKTEX_OMEGA)
    GETPARAM(param_trie_op_size, trieopsize, trie_op_size, omega::omega::trie_op_size());

    Allocate("hyfdistance", THEDATA(hyfdistance), THEDATA(trieopsize));
    Allocate("hyfnext", THEDATA(hyfnext), THEDATA(trieopsize));
    Allocate("hyfnum", THEDATA(hyfnum), THEDATA(trieopsize));
    Allocate("trie", THEDATA(trie), THEDATA(triesize));
    Allocate("trieophash", THEDATA(trieophash), 2 * THEDATA(trieopsize));
    Allocate("trieoplang", THEDATA(trieoplang), THEDATA(trieopsize));
    Allocate("trieopval", THEDATA(trieopval), THEDATA(trieopsize));
#endif // Omega

  }

public:
  void Free() override
  {
    TeXMFMemoryHandlerImpl::Free();
    TeXMFApp::FreeMemory();

    Free("linestack", THEDATA(linestack));
    Free("inputstack", THEDATA(inputstack));
    Free("inputfile", THEDATA(inputfile));
    Free("fullsourcefilenamestack", THEDATA(fullsourcefilenamestack));
    Free("sourcefilenamestack", THEDATA(sourcefilenamestack));
    Free("nest", THEDATA(nest));
    Free("savestack", THEDATA(savestack));
    Free("triec", THEDATA(triec));
    Free("triehash", THEDATA(triehash));
    Free("triel", THEDATA(triel));
    Free("trieo", THEDATA(trieo));
    Free("trier", THEDATA(trier));
    Free("trietaken", THEDATA(trietaken));

    Free("nameoffile", THEDATA(nameoffile));

#if ! defined(MIKTEX_OMEGA)
    Free("hyphword", THEDATA(hyphword));
    Free("hyphlist", THEDATA(hyphlist));
    Free("hyphlink", THEDATA(hyphlink));

    Free("trietrl", THEDATA(trietrl));
    Free("trietro", THEDATA(trietro));
    Free("trietrc", THEDATA(trietrc));

    Free("bcharlabel", THEDATA(bcharlabel));
    Free("charbase", THEDATA(charbase));
    Free("depthbase", THEDATA(depthbase));
    Free("extenbase", THEDATA(extenbase));
    Free("fontarea", THEDATA(fontarea));
    Free("fontbc", THEDATA(fontbc));
    Free("fontbchar", THEDATA(fontbchar));
    Free("fontcheck", THEDATA(fontcheck));
    Free("fontdsize", THEDATA(fontdsize));
    Free("fontec", THEDATA(fontec));
    Free("fontfalsebchar", THEDATA(fontfalsebchar));
    Free("fontglue", THEDATA(fontglue));
    Free("fontinfo", THEDATA(fontinfo));
    Free("fontname", THEDATA(fontname));
    Free("fontparams", THEDATA(fontparams));
    Free("fontsize", THEDATA(fontsize));
    Free("fontused", THEDATA(fontused));
    Free("heightbase", THEDATA(heightbase));
    Free("hyphenchar", THEDATA(hyphenchar));
    Free("italicbase", THEDATA(italicbase));
    Free("kernbase", THEDATA(kernbase));
    Free("ligkernbase", THEDATA(ligkernbase));
    Free("parambase", THEDATA(parambase));
    Free("skewchar", THEDATA(skewchar));
    Free("widthbase", THEDATA(widthbase));
#endif // not Omega

#if defined(MIKTEX_OMEGA)
    Free("hyfdistance", THEDATA(hyfdistance));
    Free("hyfnext", THEDATA(hyfnext));
    Free("hyfnum", THEDATA(hyfnum));
    Free("trie", THEDATA(trie));

    Free("trieophash", THEDATA(trieophash));
    Free("trieoplang", THEDATA(trieoplang));
    Free("trieopval", THEDATA(trieopval));
#endif
  }

public:
  void Check() override
  {
    TeXMFMemoryHandlerImpl::Check();
#if defined(THEDATA)
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(linestack));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(inputstack));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(inputfile));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(fullsourcefilenamestack));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(sourcefilenamestack));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(nest));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(savestack));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(triec));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(triehash));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(triel));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(trieo));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(trier));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(trietaken));

    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(nameoffile));

#if ! defined(MIKTEX_OMEGA)
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(hyphword));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(hyphlist));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(hyphlink));

    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(trietrl));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(trietro));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(trietrc));

    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(bcharlabel));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(charbase));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(depthbase));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(extenbase));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(fontarea));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(fontbc));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(fontbchar));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(fontcheck));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(fontdsize));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(fontec));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(fontfalsebchar));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(fontglue));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(fontinfo));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(fontname));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(fontparams));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(fontsize));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(fontused));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(heightbase));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(hyphenchar));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(italicbase));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(kernbase));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(ligkernbase));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(parambase));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(skewchar));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(widthbase));
#endif // not Omega

#if defined(MIKTEX_OMEGA)
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(hyfdistance));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(hyfnext));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(hyfnum));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(trie));

    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(trieophash));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(trieoplang));
    MIKTEX_ASSERT_VALID_HEAP_POINTER_OR_NIL(THEDATA(trieopval));
#endif
#endif // THEDATA
  }
};

MIKTEXMF_END_NAMESPACE;

#endif