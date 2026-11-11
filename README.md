# simple_tree_sitter

Tree-sitter for Eiffel: incremental parsing and syntax highlighting, with the runtime and the Eiffel, C, JSON and XML grammars built in.

It wraps [tree-sitter](https://github.com/tree-sitter/tree-sitter), a C library that parses source code into a syntax tree and re-parses only what an edit changed. It was written for the code editor of [simple_gui](https://github.com/simple-eiffel/simple_gui), and has no dependency on it. Part of the [Simple Eiffel](https://github.com/simple-eiffel) ecosystem.

> **Status: first version, not yet released.** The C wrapper and the Eiffel classes below are here and tested on macOS, Windows (MSVC) and Linux. The background worker is here too, tested on macOS so far. The adapter for simple_gui's editor is still to come. The Eiffel grammar is patched for Simple Eiffel's own source; see "Limits" for what it still does not parse.

## What it gives

- **Parsing:** text in, a syntax tree out. After an edit, a new tree that reuses the unchanged parts.
- **For an editor,** from three query files per language:
  - highlight spans: line, column, length and style;
  - fold ranges: first and last line;
  - the partner of a bracket, or of a keyword that opens or closes a block (`do` and its `end`).
- **Queries of your own:** tree-sitter's pattern language, run over a document or part of one.
- **Positions count characters.** Every position, column and length is a count of Unicode code points from 1, never bytes and never UTF-16 units. An emoji is one character.

## Example

```eiffel
local
	l_doc: TS_DOCUMENT
	l_spans: TS_SPANS
	l_changed: TS_LINE_RANGES
	i: INTEGER
do
	create l_doc.make ("eiffel")
	l_doc.set_text ({STRING_32} "class A feature x: INTEGER end")

		-- Styles for the lines on screen.
	l_spans := l_doc.spans (1, l_doc.line_count)
	from i := 1 until i > l_spans.count loop
		print (l_spans.line (i).out + ":" + l_spans.column (i).out + " "
			+ (create {TS_STYLES}).name (l_spans.style (i)) + "%N")
		i := i + 1
	end

		-- After typing: put 3 characters in place of 1, and ask what to repaint.
	l_doc.edit (7, 1, {STRING_32} "FOO")
	l_changed := l_doc.changed_lines
	from i := 1 until i > l_changed.count loop
		print (l_changed.first_line (i).out + " to " + l_changed.last_line (i).out + "%N")
		i := i + 1
	end
	l_doc.forget_changed_lines
end
```

## Classes

| Class | What it is |
| --- | --- |
| `TS_DOCUMENT` | A text, its syntax tree and its language's editor queries: `set_text`, `edit`, `spans`, `folds`, `bracket_at`, `changed_lines`, `node_at`, `new_cursor`, `captures`. |
| `TS_LANGUAGES` | The languages built in (`eiffel`, `c`, `json`, `xml`), and the language for a file name. |
| `TS_STYLES` | The eleven styles a span can have: plain, keyword, type, string, comment, number, operator, function, constant, variable, tag. |
| `TS_SPANS`, `TS_LINE_RANGES`, `TS_CAPTURES` | Results, read by index. Each frees its C memory when collected. |
| `TS_BRACKET_PAIR`, `TS_NODE_INFO` | One bracket pair; one node of the tree. |
| `TS_QUERY` | A query of your own, with the message and position of its error when it has one. |
| `TS_CURSOR` | A walk over the tree. It is valid until the document changes. |
| `TS_BLOCKS` | Text copied into a C block, for handing a text from one thread to the document's thread. |
| `TS_WORKER_CLIENT` | A text parsed in the background: the class to use from an editor. See "Parsing in the background". |
| `TS_WORKER`, `TS_RESULT_BOX` | The two halves behind the client: a document on a processor of its own, and the place its answers wait. |
| `TS_NOTIFIER` | Inherit it to be told when an answer is waiting. |

A document belongs to one thread at a time. Different documents may be used on different threads at once.

## Parsing in the background

A parse of a large text takes long enough to feel: 80 ms for 40,000 lines on the machine this was written on. `TS_WORKER_CLIENT` keeps the document on a SCOOP processor of its own, so the processor that owns the text never waits for it.

```eiffel
	-- Once.
create client.make ("eiffel", Void)
client.set_text (text)

	-- At every change of the text: this returns at once.
client.edit (l_start, l_removed, l_inserted)

	-- Later, from a timer or when the notifier was told.
client.take
if client.has_result then
		-- Recolor the lines of client.ranges from client.spans; client.folds are the folds.
	client.confirm_applied
end
```

- **Every change must be told,** in order, with `set_text` or `edit`. Positions are those of `TS_DOCUMENT`.
- **A result is for the text as it is now, or it is dropped.** `take` frees a result that was made for an older text. Nothing is lost by that: a line stays owed until `confirm_applied` says it is shown, and the next result names it again.
- **`ranges` are all the lines to recolor,** and `spans` are the spans of those lines only. A line of `ranges` with no span is plain.
- **Bracket pairs:** `ask_bracket (a_position)`, then `has_bracket` and `bracket` after a later `take`.
- **To be told instead of asking:** pass `make` an object of a class that inherits `TS_NOTIFIER`, created `separate`. Its `notify` runs on its own processor after each answer.
- **Your system must be compiled for SCOOP** (`<concurrency support="scoop"/>`) for the worker to run beside you. Without it a separate call is an ordinary call, and each change is parsed before its call returns; that way of building is not tested here.
- **Cost, measured on macOS with 40,000 lines:** `set_text` returns in 0.3 ms and `edit` in under 0.01 ms; the results of the whole text come 80 ms later. The worker's own work for one typed character in 10,000 lines is 3.7 ms.
- **Collections do not wait for a parse.** The C calls the worker makes are marked `blocking`, so EiffelStudio's collector does not wait for them. Without the mark, an allocation on the editor's processor waited up to 30 ms on that text.

## Building

The C side is one static library, built on each machine. Build output is not committed.

```
cd Clib
make            # macOS and Linux: libsimple_tree_sitter.a
make test       # the C tests
```

On Windows, from a Visual Studio x64 developer prompt:

```
cd Clib
build.bat       # simple_tree_sitter.lib
build.bat test
```

Then, with `SIMPLE_EIFFEL` set to the directory that holds this one, add the library to your ECF:

```xml
<library name="simple_tree_sitter" location="$SIMPLE_EIFFEL/simple_tree_sitter/simple_tree_sitter.ecf"/>
```

The Eiffel tests are the target `simple_tree_sitter_tests`. Run the program from this directory: it reads `testing/fixtures`.

## Layout

```
simple_tree_sitter.ecf        the library, and its tests target
Clib/
  Makefile, build.bat         the builds
  simple_tree_sitter.h, .c    the wrapper: the only header Eiffel includes
  sts_queries.c               the query files as C data (generated by `make queries`, committed)
  tools/embed_queries.c       writes sts_queries.c
  test/sts_test.c             the C tests, with a survey and a dump tool
  tree-sitter/                the runtime, vendored
  grammars/<name>/            each grammar's generated parser, vendored
  grammars/eiffel/simple_eiffel.patch   our changes to the Eiffel grammar (VERSION says how to generate the parser again)
queries/<name>/               highlights.scm, folds.scm, brackets.scm
src/                          the Eiffel classes
testing/                      the Eiffel tests and the fixtures
```

To add a grammar: copy its `src/` and LICENSE under `Clib/grammars/<name>/` with a `VERSION` file, add its three query files, add one line to the language table in `simple_tree_sitter.c`, and add it to the Makefile and `build.bat`.

## Limits

- **The Eiffel grammar is v1.0.0 with a patch of ours, and it still does not parse all Eiffel.**
  - The patch (`Clib/grammars/eiffel/simple_eiffel.patch`) adds integers in base 16, 8 and 2 (`0x1F`), character codes (`'%N'`, `'%/31/'`) and every one-character constant (`' '`, `'@'`), the attachment mark on a tuple type (`detachable TUPLE [row, col: INTEGER]`), the empty tuple (`[]`) and `separate` before a type.
  - With it, all 487 `.e` files of simple_gui, simple_widgets, simple_shell and simple_json parse without an error node (136 of them had one before).
  - Still an error node: keywords in another letter case (`false`, `END`), `unique`, a string continued over lines with `%`, an inline agent followed by its arguments (`end (x, ?)`), a real with an exponent (`1.0e308`), a `;` before the closing `note`, a feature with two `alias` names, and the `separate ... as ... do` instruction.
  - Inside an error node, keywords, strings, comments and operators keep their styles, but folds and bracket pairs across it are lost.
- **Not in this version:** one language inside another (C in an Eiffel `external`), indentation queries, grammars loaded at run time, and stopping a parse that is under way.
- A query's `#match?` patterns are a small built-in subset of regular expressions; an unsupported predicate or pattern makes the query invalid, with a message.

## Third-party notices

This library is MIT licensed (see `LICENSE`). It contains the following, each under the license kept beside it. Each is unchanged from its source, except the Eiffel grammar's parser, which is generated again from the grammar with our patch, and one line of the runtime's `lib/src/query.c` (a leak on a refused query, recorded in `Clib/tree-sitter/VERSION`):

| What | Source | Version | License | Where |
| --- | --- | --- | --- | --- |
| tree-sitter runtime | [tree-sitter/tree-sitter](https://github.com/tree-sitter/tree-sitter) | v0.27.1, commit `10eb5ac119eec9cb4267a8e035066473983f5f3a` | MIT, Copyright (c) 2018 Max Brunsfeld | `Clib/tree-sitter/` (`LICENSE`, `VERSION`) |
| Eiffel grammar | [imustafin/tree-sitter-eiffel](https://github.com/imustafin/tree-sitter-eiffel) | v1.0.0, commit `d934fb44f1d22bb76be6b56a7b2425ab3b1daf8b`, with `simple_eiffel.patch` | MIT, Copyright (c) 2024 Ilgiz Mustafin, Alessandro Schena | `Clib/grammars/eiffel/` (`LICENSE`, `VERSION`, the patch) |
| C grammar | [tree-sitter/tree-sitter-c](https://github.com/tree-sitter/tree-sitter-c) | v0.24.2, commit `b780e47fc780ddc8da13afa35a3f4ed5c157823d` | MIT, Copyright (c) 2014 Max Brunsfeld | `Clib/grammars/c/` |
| JSON grammar | [tree-sitter/tree-sitter-json](https://github.com/tree-sitter/tree-sitter-json) | v0.24.8, commit `ee35a6ebefcef0c5c416c0d1ccec7370cfca5a24` | MIT, Copyright (c) 2014 Max Brunsfeld | `Clib/grammars/json/` |
| XML grammar | [tree-sitter-grammars/tree-sitter-xml](https://github.com/tree-sitter-grammars/tree-sitter-xml) | v0.7.0, commit `4b64dd3a03ec002258d6268d712fd93716d6ab57` | MIT, Copyright (c) 2023 ObserverOfTime | `Clib/grammars/xml/`, and `Clib/grammars/common/scanner.h` |

- **Why the runtime is compiled in** rather than linked from Homebrew's or the system's shared `libtree-sitter` (the fleet's rule, BUILD_STANDARDS section 9: no second *shared* copy of a library the system provides; source compiled in is allowed with its reason stated):
  - The grammars' generated parsers are made for this runtime's parser format (14, v0.27.1), and the runtime must match them, whatever version a system has installed.
  - It carries the one-line `query.c` fix above until upstream has it.
  - Compiled into this library's own object, it cannot collide with another program's shared `libtree-sitter`.
- The tree-sitter runtime carries two notices of its own: Unicode, Inc.'s for the ICU headers in `Clib/tree-sitter/lib/src/unicode/` (its `LICENSE` is there), and musl's (MIT) for `Clib/tree-sitter/lib/src/wasm-stdlib/`, which this library does not compile.
- `queries/c/highlights.scm`, `queries/json/highlights.scm` and `queries/xml/highlights.scm` began as the `highlights.scm` of the grammar they serve (MIT, as above) and were changed for this library's capture names. The Eiffel queries and every `folds.scm` and `brackets.scm` were written for this library.
