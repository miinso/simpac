window.BENCHMARK_DATA = {
  "lastUpdate": 1777635347671,
  "repoUrl": "https://github.com/miinso/simpac",
  "entries": {
    "cloth-implicit-euler": [
      {
        "commit": {
          "author": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "committer": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "distinct": true,
          "id": "029479bb382947788d509fa8b51b5d2015565811",
          "message": "chore: update readme",
          "timestamp": "2026-03-10T18:45:55+09:00",
          "tree_id": "d39b2e8d0601f56da41a2f66038123b54075e042",
          "url": "https://github.com/miinso/simpac/commit/029479bb382947788d509fa8b51b5d2015565811"
        },
        "date": 1773136362020,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ClothImplicit/50/real_time",
            "value": 10.866552591323853,
            "unit": "ms/iter",
            "extra": "iterations: 192\ncpu: 10.86557984375 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/100/real_time",
            "value": 45.601763623826045,
            "unit": "ms/iter",
            "extra": "iterations: 47\ncpu: 45.59689274468084 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/200/real_time",
            "value": 199.76438175548208,
            "unit": "ms/iter",
            "extra": "iterations: 11\ncpu: 199.74796354545452 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "committer": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "distinct": true,
          "id": "f61719a8a954f611939f164627bab051fa3e9d7e",
          "message": "chore: update changelog (r8-r12)",
          "timestamp": "2026-03-10T19:15:41+09:00",
          "tree_id": "fc5ab180045eed340dd81b62d8a73b7a84d443d2",
          "url": "https://github.com/miinso/simpac/commit/f61719a8a954f611939f164627bab051fa3e9d7e"
        },
        "date": 1773137804175,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ClothImplicit/50/real_time",
            "value": 11.006629903903182,
            "unit": "ms/iter",
            "extra": "iterations: 191\ncpu: 11.005418209424084 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/100/real_time",
            "value": 45.31521492816032,
            "unit": "ms/iter",
            "extra": "iterations: 47\ncpu: 45.311452957446825 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/200/real_time",
            "value": 197.0595359802246,
            "unit": "ms/iter",
            "extra": "iterations: 10\ncpu: 197.03825690000016 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "committer": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "distinct": true,
          "id": "30b79fe2bd777a73d007ee19d1964e28ffb1306b",
          "message": "chore: update changelog",
          "timestamp": "2026-03-10T19:18:22+09:00",
          "tree_id": "fc5ab180045eed340dd81b62d8a73b7a84d443d2",
          "url": "https://github.com/miinso/simpac/commit/30b79fe2bd777a73d007ee19d1964e28ffb1306b"
        },
        "date": 1773137973590,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ClothImplicit/50/real_time",
            "value": 11.127261405295513,
            "unit": "ms/iter",
            "extra": "iterations: 188\ncpu: 11.125959021276598 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/100/real_time",
            "value": 47.59084636514837,
            "unit": "ms/iter",
            "extra": "iterations: 44\ncpu: 47.58538695454545 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/200/real_time",
            "value": 198.56536388397217,
            "unit": "ms/iter",
            "extra": "iterations: 10\ncpu: 198.54471670000004 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "committer": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "distinct": true,
          "id": "fb5d0000935c5f2117a012d5d444e646b0400a02",
          "message": "refactor(cloth): extract shared sim infra into setup.h\n\n~92 lines of duplicated boilerplate (sim:: globals, rebuild/gather/scatter,\ncomponent registration, observers, scene loading) extracted from\nmain_implicit.cpp and main_symplectic.cpp into setup.h. solver files\nnow only contain algorithm-specific code.\n\nalso reorganizes directory structure: components/core.h split into\nparticle.h + constraint.h, physics/ moved to sim/ + math/, systems/render/\nmoved to render/.",
          "timestamp": "2026-04-01T19:04:27+09:00",
          "tree_id": "0db8e4077e802aa58ab356ee28a73ce2fb938a60",
          "url": "https://github.com/miinso/simpac/commit/fb5d0000935c5f2117a012d5d444e646b0400a02"
        },
        "date": 1775037979163,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ClothImplicit/50/real_time",
            "value": 8.697965193767937,
            "unit": "ms/iter",
            "extra": "iterations: 245\ncpu: 8.696758808163265 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/100/real_time",
            "value": 36.373201169465716,
            "unit": "ms/iter",
            "extra": "iterations: 57\ncpu: 36.36778803508771 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/200/real_time",
            "value": 153.77889360700334,
            "unit": "ms/iter",
            "extra": "iterations: 14\ncpu: 153.7554867142857 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "committer": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "distinct": true,
          "id": "ce2598e1728da3003857916d2598260e4d064dda",
          "message": "feat(cloth): add state buffers",
          "timestamp": "2026-03-31T19:09:39+09:00",
          "tree_id": "a840294afd07faf929b9f162eea8e58f5bb49ce6",
          "url": "https://github.com/miinso/simpac/commit/ce2598e1728da3003857916d2598260e4d064dda"
        },
        "date": 1775037988413,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ClothImplicit/50/real_time",
            "value": 8.43847370147705,
            "unit": "ms/iter",
            "extra": "iterations: 250\ncpu: 8.437708248 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/100/real_time",
            "value": 35.57236719939668,
            "unit": "ms/iter",
            "extra": "iterations: 59\ncpu: 35.56839281355933 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/200/real_time",
            "value": 157.6227774986854,
            "unit": "ms/iter",
            "extra": "iterations: 13\ncpu: 157.59828776923067 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "committer": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "distinct": true,
          "id": "ce2598e1728da3003857916d2598260e4d064dda",
          "message": "feat(cloth): add state buffers",
          "timestamp": "2026-03-31T19:09:39+09:00",
          "tree_id": "a840294afd07faf929b9f162eea8e58f5bb49ce6",
          "url": "https://github.com/miinso/simpac/commit/ce2598e1728da3003857916d2598260e4d064dda"
        },
        "date": 1775098586137,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ClothImplicit/50/real_time",
            "value": 8.395440022464797,
            "unit": "ms/iter",
            "extra": "iterations: 253\ncpu: 8.394836268774705 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/100/real_time",
            "value": 35.030905405680336,
            "unit": "ms/iter",
            "extra": "iterations: 60\ncpu: 35.027739083333344 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/200/real_time",
            "value": 153.94287842970627,
            "unit": "ms/iter",
            "extra": "iterations: 13\ncpu: 153.9288237692307 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "committer": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "distinct": true,
          "id": "de286aab311c62f7e60b04c72b9a7891d7f1b0bf",
          "message": "lab(cloth): add corot energy",
          "timestamp": "2026-04-14T20:19:04+09:00",
          "tree_id": "cc1ded86edcbc1dcfdf3711d07d9819c3eb84d8f",
          "url": "https://github.com/miinso/simpac/commit/de286aab311c62f7e60b04c72b9a7891d7f1b0bf"
        },
        "date": 1776165798403,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ClothImplicit/50/real_time",
            "value": 8.450095176696777,
            "unit": "ms/iter",
            "extra": "iterations: 250\ncpu: 8.449248168 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/100/real_time",
            "value": 35.1106325785319,
            "unit": "ms/iter",
            "extra": "iterations: 60\ncpu: 35.10770206666667 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/200/real_time",
            "value": 155.70335728781563,
            "unit": "ms/iter",
            "extra": "iterations: 14\ncpu: 155.68705142857158 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "committer": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "distinct": true,
          "id": "e7242b4cc4935e3576669b65c38f95b03f28426d",
          "message": "feat(cloth): add triangle renderer",
          "timestamp": "2026-04-16T18:08:02+09:00",
          "tree_id": "86a232228f3ffaa29ee42ee30abf9274747f92b5",
          "url": "https://github.com/miinso/simpac/commit/e7242b4cc4935e3576669b65c38f95b03f28426d"
        },
        "date": 1776330664845,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ClothImplicit/50/real_time",
            "value": 8.206908767287796,
            "unit": "ms/iter",
            "extra": "iterations: 259\ncpu: 8.206261482625482 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/100/real_time",
            "value": 33.887032539613784,
            "unit": "ms/iter",
            "extra": "iterations: 62\ncpu: 33.88264282258065 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/200/real_time",
            "value": 151.0910987854004,
            "unit": "ms/iter",
            "extra": "iterations: 14\ncpu: 151.0725489285714 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "committer": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "distinct": true,
          "id": "b3deb1fc5b5ff9a727b69658151a3974b28674d4",
          "message": "feat(cloth): add triangle interaction\n\npick/drag/hover for triangles mirroring particle interaction.",
          "timestamp": "2026-04-16T20:15:22+09:00",
          "tree_id": "ba1944492b385def4911caf9cdbea9ad507328fb",
          "url": "https://github.com/miinso/simpac/commit/b3deb1fc5b5ff9a727b69658151a3974b28674d4"
        },
        "date": 1776338252068,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ClothImplicit/50/real_time",
            "value": 6.5733061073729715,
            "unit": "ms/iter",
            "extra": "iterations: 322\ncpu: 6.57259950931677 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/100/real_time",
            "value": 28.393634378093562,
            "unit": "ms/iter",
            "extra": "iterations: 73\ncpu: 28.3885045890411 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/200/real_time",
            "value": 129.71928715705872,
            "unit": "ms/iter",
            "extra": "iterations: 16\ncpu: 129.6992600625 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "committer": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "distinct": true,
          "id": "058a52be32760010ad4a4eee88ad4f17c343dc46",
          "message": "fix(cloth): emcc expects lvalue ref",
          "timestamp": "2026-04-16T20:31:08+09:00",
          "tree_id": "a368239d49df0d9654aa8c5ba4d78a48ba925bb4",
          "url": "https://github.com/miinso/simpac/commit/058a52be32760010ad4a4eee88ad4f17c343dc46"
        },
        "date": 1776339134441,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ClothImplicit/50/real_time",
            "value": 8.248272396269298,
            "unit": "ms/iter",
            "extra": "iterations: 252\ncpu: 8.246992603174602 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/100/real_time",
            "value": 34.4053122305101,
            "unit": "ms/iter",
            "extra": "iterations: 62\ncpu: 34.400185596774186 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/200/real_time",
            "value": 155.00221933637346,
            "unit": "ms/iter",
            "extra": "iterations: 14\ncpu: 154.98753635714291 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "committer": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "distinct": true,
          "id": "34a38b71416f16c733d4e5d701debbe2085a50e7",
          "message": "fix(cloth): cache triangle rest pose on first build",
          "timestamp": "2026-04-30T19:03:32+09:00",
          "tree_id": "ab72df1de68e05f9d390827d104a127e73d61268",
          "url": "https://github.com/miinso/simpac/commit/34a38b71416f16c733d4e5d701debbe2085a50e7"
        },
        "date": 1777544743570,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ClothImplicit/50/real_time",
            "value": 8.483219906628369,
            "unit": "ms/iter",
            "extra": "iterations: 251\ncpu: 8.482331378486055 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/100/real_time",
            "value": 36.015752951304115,
            "unit": "ms/iter",
            "extra": "iterations: 60\ncpu: 36.01033873333335 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/200/real_time",
            "value": 157.41572013268103,
            "unit": "ms/iter",
            "extra": "iterations: 13\ncpu: 157.40098407692324 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "committer": {
            "email": "minseo.park.cs@gmail.com",
            "name": "Minseo Park",
            "username": "miinso"
          },
          "distinct": true,
          "id": "eaf2c37e3f542cb45f6ac8efbf7dc1ce5e31cfbf",
          "message": "fix(cloth): gate solvers on particle_flags not inv_mass",
          "timestamp": "2026-05-01T20:33:10+09:00",
          "tree_id": "9bb1f6ccb3e54ce8d6223f6028161ff22a61ca87",
          "url": "https://github.com/miinso/simpac/commit/eaf2c37e3f542cb45f6ac8efbf7dc1ce5e31cfbf"
        },
        "date": 1777635346311,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ClothImplicit/50/real_time",
            "value": 8.519776405826692,
            "unit": "ms/iter",
            "extra": "iterations: 248\ncpu: 8.518623338709677 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/100/real_time",
            "value": 37.3437191758837,
            "unit": "ms/iter",
            "extra": "iterations: 56\ncpu: 37.33773987500001 ms\nthreads: 1"
          },
          {
            "name": "BM_ClothImplicit/200/real_time",
            "value": 158.37458463815543,
            "unit": "ms/iter",
            "extra": "iterations: 13\ncpu: 158.3543263846155 ms\nthreads: 1"
          }
        ]
      }
    ]
  }
}