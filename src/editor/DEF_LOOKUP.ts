import fs from "node:fs";

class Context {
  left: (Glyph | Group)[] = [];
  right: (Glyph | Group)[] = [];

  toCode(): string {
    let s = `make_shared<Lookup::Context>(initializer_list<variant<shared_ptr<Glyph>, shared_ptr<Group>>>({`;
    s += this.left.map(it => it.toCode()).join(", ");
    s += `}), initializer_list<variant<shared_ptr<Glyph>, shared_ptr<Group>>>({`;
    s += this.right.map(it => it.toCode()).join(", ");
    s += `})),`;
    return s;
  }
}

class Lookup {
  name?: string;
  base?: "skip" | "process";
  marks?: "skip" | "all" | Group | Glyph;
  exceptContext?: Context;
  inContext?: Context;
  substList?: Subst[];
  attach?: Attach;
  adjustSingle?: AdjustSingle;

  toCode(): string {
    if (this.name === undefined) {
      throw new Error();
    }
    const indent = " ".repeat(20);
    let s = `make_shared<Lookup>(`;
    switch (this.base) {
      case "skip":
        s += "Lookup::SkipBase{},";
        break;
      case "process":
        s += "Lookup::ProcessBase{},";
        break;
      default:
        throw new Error();
    }
    s += "\n";
    s += indent;
    if (this.marks === "skip") {
      s += "Lookup::SkipMarks{},";
    } else if (this.marks === "all") {
      s += "Lookup::ProcessMarks(Lookup::ProcessMarks::All{}),";
    } else if (this.marks instanceof Glyph) {
      s += `Lookup::ProcessMarks(Lookup::ProcessMarks::MarkGlyphs({getGlyphByName("${this.marks.name}")})),`
    } else if (this.marks instanceof Group) {
      s += `Lookup::ProcessMarks(Lookup::ProcessMarks::MarkGroup(getGroupByName("${this.marks.name}"))),`
    } else {
      throw new Error();
    }
    s += "\n" + indent;
    if (this.exceptContext) {
      s += this.exceptContext.toCode();
    } else {
      s += "nullptr,";
    }
    s += "\n" + indent;
    if (this.inContext) {
      s += this.inContext.toCode();
    } else {
      s += "nullptr,";
    }
    s += "\n" + indent;
    if (this.attach) {
      s += this.attach.toCode();
    } else {
      s += "nullptr,";
    }
    s += "\n" + indent;
    if (this.adjustSingle) {
      s += this.adjustSingle.toCode();
    } else {
      s += "nullptr,";
    }
    s += "\n" + indent;
    s += Subst.ToCode(this.substList ?? []);
    s += ");";
    return `lookups[${this.name}] = ${s}`;
  }
}

class Glyph {
  readonly name: string;

  constructor(name: string) {
    this.name = unquote(name);
  }

  toCode(): string {
    return `getGlyphByName("${this.name}")`;
  }
}

class Group {
  readonly name: string;

  constructor(name: string) {
    this.name = unquote(name);
  }

  toCode(): string {
    return `getGroupByName("${this.name}")`;
  }
}

class Subst {
  input: (Glyph | Group)[] = [];
  output: (Glyph | Group)[] = [];

  static ToCode(list: Subst[]): string {
    let s = "initializer_list<shared_ptr<Lookup::Substitution>>({";
    s += list.map(subst => subst.toCode()).join(", ");
    s += "})";
    return s;
  }

  toCode() : string {
    let s = `make_shared<Lookup::Substitution>(initializer_list<variant<shared_ptr<Glyph>, shared_ptr<Group>>>({`;
    s += this.input.map(it => it.toCode()).join(", ");
    s += `}), `;
    s += `initializer_list<variant<shared_ptr<Glyph>, shared_ptr<Group>>>({`;
    s += this.output.map((it => it.toCode())).join(", ");
    s += `}))`;
    return s;
  }
}

const trim = (s: string) => {
  s = s.trim();
  while (true) {
    const n = s.replaceAll("  ", " ");
    if (n === s) {
      return n;
    }
    s = n;
  }
}

function unquote(s: string) : string {
  if (s.startsWith(`"`) !== s.endsWith(`"`)) {
    throw new Error();
  }
  if (s.startsWith(`"`)) {
    return s.substring(1, s.length - 1);
  } else {
    return s;
  }
}

const consumeContext = (lines: string[], start: number) : {next: number, context: Context } => {
  const first = trim(lines[start]);
  if (first !== "EXCEPT_CONTEXT" && first !== "IN_CONTEXT") {
    throw new Error();
  }
  const context = new Context();
  for (let i = start + 1; i < lines.length; ) {
    const l = lines[i];
    if (l === "END_CONTEXT") {
      return {next: i + 1, context};
    } else {
      const tokens = trim(l).split(" ");
      while (tokens.length > 0) {
        const op = tokens.shift();
        switch (op) {
          case "LEFT": {
            const type = tokens.shift();
            const name = tokens.shift();
            if (name === undefined) {
              throw new Error();
            }
            if (type === "GLYPH") {
              context.left.push(new Glyph(name));
            } else if (type === "GROUP") {
              context.left.push(new Group(name));
            } else {
              throw new Error();
            }
            break;
          }
          case "RIGHT": {
            const type = tokens.shift();
            const name = tokens.shift();
            if (name === undefined) {
              throw new Error();
            }
            if (type === "GLYPH") {
              context.right.push(new Glyph(name));
            } else if (type === "GROUP") {
              context.right.push(new Group(name));
            } else {
              throw new Error();
            }
            break;
          }
          default:
            console.log(`l=${l}; op=${op}`, tokens);
            throw new Error();
        }
      }
      i += 1;
    }
  }
  return { next: lines.length, context};
}

const consumeSub = (lines: string[], start: number): { next: number, subst: Subst } => {
  const first = trim(lines[start]);
  const tokens = first.split(" ");
  tokens.shift();
  const subst = new Subst();
  while (tokens.length > 0) {
    const op = tokens.shift();
    if (op === "GLYPH") {
      const name = tokens.shift();
      if (name === undefined) {
        throw new Error();
      }
      subst.input.push(new Glyph(name));
    } else if (op === "GROUP") {
      const name = tokens.shift();
      if (name === undefined) {
        throw new Error();
      }
      subst.input.push(new Group(name));
    } else {
      console.log(`op=${op}`);
      throw new Error();
    }
  }
  for (let i = start + 1; i < lines.length;) {
    const l = trim(lines[i]);
    if (l.startsWith("WITH")) {
      const tokens = trim(l).split(" ");
      if (tokens.shift() !== "WITH") throw new Error();
      while (tokens.length > 0) {
        const op = tokens.shift();
        const name = tokens.shift();
        if (name === undefined) {
          throw new Error();
        }
        if (op === "GLYPH") {
          subst.output.push(new Glyph(name));
        } else if (op === "GROUP") {
          subst.output.push(new Group(name));
        } else {
          throw new Error();
        }
      }
      i += 1;
    } else if (l === "END_SUB") {
      return {next: i + 1, subst };
    } else {
      console.log(l);
      throw new Error();
    }
  }
  return{next: lines.length, subst };
};

const consumeSubstitution = (lines: string[], start: number, lookup: Lookup) : { next: number, lookup: Lookup } => {
  const first = lines[start];
  if (first !== "AS_SUBSTITUTION") {
    throw new Error();
  }
  for (let i = start + 1; i < lines.length;) {
    const l = trim(lines[i]);
    if (l === "END_SUBSTITUTION") {
      return { next: i + 1, lookup };
    } else if (l.startsWith("SUB ")) {
      const { next, subst } = consumeSub(lines, i);
      lookup.substList!.push(subst);
      i = next;
    } else {
      console.log(l);
      throw new Error();
    }
  }
  return {next: lines.length, lookup };
};

class Attach {
  input: (Glyph | Group)[] = [];
  output: ({target: Glyph | Group, anchor: string})[] = [];

  toCode(): string {
    let s = `make_shared<Lookup::Attach>(initializer_list<variant<shared_ptr<Glyph>, shared_ptr<Group>>>({`;
    s += this.input.map(it => it.toCode()).join(", ");
    s += "}), initializer_list<Lookup::AttachTarget>({";
    s += this.output.map(it => {
      return `Lookup::AttachTarget(${it.target.toCode()}, getAnchorByName("${it.anchor}"))`;
    }).join(", ");
    s += "})),";
    return s;
  }
}

const consumeAttach = (lines: string[], start: number) : {next: number, attach: Attach } => {
  const first = trim(lines[start]).split(" ");
  if (first.shift() !== "ATTACH") throw new Error();
  const attach = new Attach();
  const op = first.shift();
  const name = first.shift();
  if (name === undefined) throw new Error();
  if (op === "GROUP") {
    attach.input.push(new Group(name));
  } else if (op === "GLYPH") {
    attach.input.push(new Glyph(name));
  } else {
    throw new Error();
  }
  for (let i = start + 1; i < lines.length;) {
    const l = trim(lines[i]);
    if (l === "END_ATTACH") {
      return {next: i + 1, attach };
    } else if (l.startsWith("TO GROUP ") || l.startsWith("TO GLYPH ") || l.startsWith("GROUP ") || l.startsWith("GLYPH ")) {
      const tokens = l.split(" ");
      if (l.startsWith("TO GROUP ") || l.startsWith("TO GLYPH ")) {
        const to = tokens.shift();
        if (to !== "TO") throw new Error();
      }
      const type = tokens.shift();
      const name = tokens.shift();
      if (name === undefined) {
        throw new Error();
      }
      const at = tokens.shift();
      if (at !== "AT") throw new Error();
      const anchor = tokens.shift();
      if (anchor !== "ANCHOR") throw new Error();
      const anchorName = tokens.shift();
      if (anchorName === undefined) throw new Error();
      if (type === "GROUP") {
        attach.output.push({target: new Group(name), anchor: unquote(anchorName)});
      } else if (type === "GLYPH") {
        attach.output.push({target: new Glyph(name), anchor: unquote(anchorName)});
      } else {
        throw new Error();
      }
      i += 1;
    } else {
      console.log(l);
      throw new Error();
    }
  }
  return {next:lines.length, attach};
};

class AdjustSingle {
  glyphs: {name: string, dx?: number, dy?: number }[] = [];

  toCode(): string {
    let s = `make_shared<Lookup::AdjustSingle>(initializer_list<Lookup::AdjustGlyph>({`;
    s += this.glyphs.map(({name, dx, dy}) => {
      let k = `Lookup::AdjustGlyph("${unquote(name)}", `;
      if (dx === undefined) {
        k += "nullopt, ";
      } else {
        k += `(int16_t)${dx}, `;
      }
      if (dy === undefined) {
        k += "nullopt)";
      } else {
        k += `(int16_t)${dy})`;
      }
      return k;
    }).join(", ");
    s += "})),";
    return s;
  }
}

const consumeAdjustSingle = (lines: string[], start: number): {next: number, adjustSingle: AdjustSingle} => {
  const tokens = trim(lines[start]).split(" ");
  if (tokens.shift() !== "ADJUST_SINGLE") {
    throw new Error();
  }
  const adjustSingle = new AdjustSingle();
  while (tokens.length > 0) {
    const what = tokens.shift();
    const name = tokens.shift();
    if (name === undefined) throw new Error();
    const by = tokens.shift();
    if (by !== "BY") throw new Error();
    const pos = tokens.shift();
    if (pos !== "POS") throw new Error();
    let dx: number | undefined = undefined;
    let dy: number | undefined = undefined;
    while (tokens.length > 0) {
      const type = tokens.shift();
      if (type === "END_POS") {
        break;
      }
      const value = tokens.shift();
      if (value === undefined) throw new Error();
      if (type === "DX") {
        dx = parseInt(value, 10);
      } else if (type === "DY") {
        dy = parseInt(value, 10);
      } else {
        throw new Error();
      }
    }
    if (what === "GLYPH") {
      adjustSingle.glyphs.push({name, dx, dy});
    } else {
      throw new Error();
    }
  }
  const end = trim(lines[start + 1]);
  if (end !== "END_ADJUST") {
    throw new Error();
  }
  return { next: start + 2, adjustSingle };
};

const consumePosition = (lines: string[], start: number, lookup: Lookup) : {next: number, lookup: Lookup} => {
  const first = trim(lines[start]);
  if (first !== "AS_POSITION") {
    throw new Error();
  }
  for (let i = start + 1; i < lines.length; ) {
    const l = trim(lines[i]);
    if (l === "END_POSITION") {
      return {next: i + 1, lookup};
    } else if (l.startsWith("ATTACH")) {
      const { next, attach } = consumeAttach(lines, i);
      lookup.attach = attach;
      i = next;
    } else if (l.startsWith("ADJUST_SINGLE")) {
      const { next, adjustSingle } = consumeAdjustSingle(lines, i);
      lookup.adjustSingle = adjustSingle;
      i = next;
    } else {
      console.log(l);
      throw new Error();
    }
  }
  return {next:lines.length, lookup};
}

const consumeLookup = (lines: string[], start: number): { next: number, lookup: Lookup } => {
  const first = lines[start];
  if (!first.startsWith("DEF_LOOKUP")) {
    throw new Error();
  }
  const lookup = new Lookup();
  const tokens = trim(first).split(" ");
  tokens.shift();
  const name = tokens.shift();
  if (name === undefined) {
    throw new Error();
  }
  lookup.name = name;
  switch (tokens.shift()) {
    case "PROCESS_BASE":
      lookup.base = "process";
      break;
    case "SKIP_BASE":
      lookup.base = "skip";
      break;
    default:
      throw new Error();
  }
  switch (tokens.shift()) {
    case "PROCESS_MARKS": {
      const marksWhat = tokens.shift();
      if (marksWhat === "MARK_GLYPH_SET") {
        const n = tokens.shift();
        if (n === undefined) {
          throw new Error();
        }
        lookup.marks = new Glyph(n);
      } else if (marksWhat === "ALL" || marksWhat === `"ALL"`) {
        lookup.marks = "all";
      } else if (marksWhat?.startsWith(`"`)) {
        lookup.marks = new Group(marksWhat);
      } else {
        throw new Error();
      }
      break;
    }
    case "SKIP_MARKS":
      lookup.marks = "skip";
      break;
    default:
      throw new Error();
  }
  if (tokens.shift() !== "DIRECTION") {
    throw new Error();
  }
  const directionWhat = tokens.shift();
  if (directionWhat !== "LTR") {
    throw new Error();
  }
  for (let i = start + 1; i < lines.length;) {
    const l = trim(lines[i]);
    if (l === "END_SUBSTITUTION" || l === "END_POSITION") {
      return {next: i + 1, lookup };
    } else if (l === "EXCEPT_CONTEXT") {
      const { next, context } = consumeContext(lines, i);
      lookup.exceptContext = context;
      i = next;
    } else if (l === "IN_CONTEXT") {
      const { next, context } = consumeContext(lines, i);
      lookup.inContext = context;
      i = next;
    } else if (l === "AS_SUBSTITUTION") {
      lookup.substList = [];
      return consumeSubstitution(lines, i, lookup);
    } else if (l === "AS_POSITION") {
      return consumePosition(lines, i, lookup);
    } else {
      console.log(l);
      throw new Error();
    }
  }
  return { next: lines.length, lookup };
};

const main = async () => {
  const vtp = (await fs.promises.readFile("EgyptianText_200.vtp")).toString("utf-8");
  const lines = vtp.split("\n");
  for (let i = 0; i < lines.length;) {
    const l = lines[i];
    if (l.startsWith("DEF_LOOKUP")) {
      const { next, lookup } = consumeLookup(lines, i);
      console.log(lookup.toCode());
      i = next;
    } else {
      i += 1;
    }
  }
};

main();
