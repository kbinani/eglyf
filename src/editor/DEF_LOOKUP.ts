import fs from "node:fs";

const trim = (s: string) => {
  s = s.trim();
  while (true) {
    const n = s.replaceAll("  ", " ");
    if (n === s) {
      return n;
    }
    s = n;
  }
};

const consumeContext = (lines: string[], start: number) :number => {
  const first = trim(lines[start]);
  if (first !== "EXCEPT_CONTEXT" && first !== "IN_CONTEXT") {
    throw new Error();
  }
  console.log("->exceptContext()");
  for (let i = start + 1; i < lines.length; ) {
    const l = lines[i];
    if (l === "END_CONTEXT") {
      console.log("->endContext()");
      return i + 1;
    } else {
      const tokens = trim(l).split(" ");
      while (tokens.length > 0) {
        const op = tokens.shift();
        switch (op) {
          case "LEFT": {
            let s = "";
            const type = tokens.shift();
            if (type === "GLYPH") {
              s += "->leftGlyph(";
            } else if (type === "GROUP") {
              s += "->leftGroup(";
            } else {
              throw new Error();
            }
            const name = tokens.shift();
            s += `${name})`;
            console.log(s);
            break;
          }
          case "RIGHT": {
            let s = "";
            const type = tokens.shift();
            if (type === "GLYPH") {
              s += "->rightGlyph(";
            } else if (type === "GROUP") {
              s += "->rightGroup(";
            } else {
              throw new Error();
            }
            const name = tokens.shift();
            s += `${name})`;
            console.log(s);
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
  return lines.length;
}

const consumeSub = (lines: string[], start: number): number => {
  const first = trim(lines[start]);
  const tokens = first.split(" ");
  tokens.shift();
  let s = "->substitute()";
  while (tokens.length > 0) {
    const op = tokens.shift();
    if (op === "GLYPH") {
      const name = tokens.shift();
      s += `->subGlyph(${name})`;
    } else if (op === "GROUP") {
      const name = tokens.shift();
      s += `->subGroup(${name})`;
    } else {
      console.log(`op=${op}`);
      throw new Error();
    }
  }
  console.log(s);
  for (let i = start + 1; i < lines.length;) {
    const l = trim(lines[i]);
    if (l.startsWith("WITH")) {
      const tokens = trim(l).split(" ");
      if (tokens.shift() !== "WITH") throw new Error();
      let s = "";
      while (tokens.length > 0) {
        const op = tokens.shift();
        const name = tokens.shift();
        if (name === undefined) {
          throw new Error();
        }
        if (op === "GLYPH") {
          s += `->withGlyph(${name})`;
        } else if (op === "GROUP") {
          s += `->withGroup(${name})`;
        } else {
          throw new Error();
        }
      }
      console.log(s);
      i += 1;
    } else if (l === "END_SUB") {
      console.log(`->endSub()`);
      return i + 1;
    } else {
      console.log(l);
      throw new Error();
    }
  }
  return lines.length;
};

const consumeSubstitution = (lines: string[], start: number) : number => {
  const first = lines[start];
  if (first !== "AS_SUBSTITUTION") {
    throw new Error();
  }
  console.log("->asSubstitution()");
  for (let i = start + 1; i < lines.length;) {
    const l = trim(lines[i]);
    if (l === "END_SUBSTITUTION") {
      console.log("->endSubstitutionLookup();");
      return i + 1;
    } else if (l.startsWith("SUB ")) {
      i = consumeSub(lines, i);
    } else {
      console.log(l);
      throw new Error();
    }
  }
  return lines.length;
};

const consumeAttach = (lines: string[], start: number) : number => {
  const first = trim(lines[start]).split(" ");
  if (first.shift() !== "ATTACH") throw new Error();
  const op = first.shift();
  const name = first.shift();
  if (name === undefined) throw new Error();
  if (op === "GROUP") {
    console.log(`->attachGroup(${name})`);
  } else if (op === "GLYPH") {
    console.log(`->attachGlyph(${name})`);
  } else {
    console.log(lines[start]);
    throw new Error();
  }
  for (let i = start + 1; i < lines.length;) {
    const l = trim(lines[i]);
    if (l === "END_ATTACH") {
      console.log(`->endAttach()`);
      return i + 1;
    } else if (l.startsWith("TO GROUP ") || l.startsWith("TO GLYPH ") || l.startsWith("GROUP ") || l.startsWith("GLYPH ")) {
      const tokens = l.split(" ");
      if (l.startsWith("TO GROUP ") || l.startsWith("TO GLYPH ")) {
        const to = tokens.shift();
        if (to !== "TO") throw new Error();
      }
      const type = tokens.shift();
      const name = tokens.shift();
      const at = tokens.shift();
      if (at !== "AT") throw new Error();
      const anchor = tokens.shift();
      if (anchor !== "ANCHOR") throw new Error();
      const anchorName = tokens.shift();
      if (type === "GROUP") {
        console.log(`->toGroup(${name}, ${anchorName})`);
      } else if (type === "GLYPH") {
        console.log(`->toGlyph(${name}, ${anchorName})`);
      } else {
        throw new Error();
      }
      i += 1;
    } else {
      console.log(l);
      throw new Error();
    }
  }
  return lines.length;
};

const consumeAdjustSingle = (lines: string[], start: number): number => {
  const tokens = trim(lines[start]).split(" ");
  if (tokens.shift() !== "ADJUST_SINGLE") {
    throw new Error();
  }
  console.log("->adjustSingle()");
  while (tokens.length > 0) {
    const what = tokens.shift();
    const name = tokens.shift();
    if (name === undefined) throw new Error();
    const by = tokens.shift();
    if (by !== "BY") throw new Error();
    const pos = tokens.shift();
    if (pos !== "POS") throw new Error();
    let dx = "nullopt";
    let dy = "nullopt";
    while (tokens.length > 0) {
      const type = tokens.shift();
      if (type === "END_POS") {
        break;
      }
      const value = tokens.shift();
      if (value === undefined) throw new Error();
      if (type === "DX") {
        dx = value;
      } else if (type === "DY") {
        dy = value;
      } else {
        throw new Error();
      }
    }
    if (what === "GLYPH") {
      console.log(`->adjustGlyph(${name}, ${dx}, ${dy})`);
    } else {
      throw new Error();
    }
  }
  const end = trim(lines[start + 1]);
  if (end !== "END_ADJUST") {
    throw new Error();
  }
  console.log(`->endAdjust()`);
  return start + 2;
};

const consumePosition = (lines: string[], start: number) : number => {
  const first = trim(lines[start]);
  if (first !== "AS_POSITION") {
    throw new Error();
  }
  console.log("->asPosition()");
  for (let i = start + 1; i < lines.length; ) {
    const l = trim(lines[i]);
    if (l === "END_POSITION") {
      console.log("->endPositionLookup();");
      return i + 1;
    } else if (l.startsWith("ATTACH")) {
      i = consumeAttach(lines, i);
    } else if (l.startsWith("ADJUST_SINGLE")) {
      i = consumeAdjustSingle(lines, i);
    } else {
      console.log(l);
      throw new Error();
    }
  }
  return lines.length;
}

const consumeLookup = (lines: string[], start: number): number => {
  const first = lines[start];
  if (!first.startsWith("DEF_LOOKUP")) {
    throw new Error();
  }
  const tokens = trim(first).split(" ");
  tokens.shift();
  const name = tokens.shift();
  let base: string;
  switch (tokens.shift()) {
    case "PROCESS_BASE":
      base = "processBase()";
      break;
    case "SKIP_BASE":
      base = "skipBase()";
      break;
    default:
      throw new Error();
  }
  let marks: string;
  switch (tokens.shift()) {
    case "PROCESS_MARKS": {
      const marksWhat = tokens.shift();
      if (marksWhat === "MARK_GLYPH_SET") {
        marks = `processMarkGlyphs(${tokens.shift()})`;
      } else if (marksWhat === "ALL" || marksWhat === `"ALL"`) {
        marks = "processMarksAll()";
      } else if (marksWhat?.startsWith(`"`)) {
        marks = `processMarkGroup(${marksWhat})`;
      } else {
        throw new Error();
      }
      break;
    }
    case "SKIP_MARKS":
      marks = "skipMarks()";
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
  console.log(`defineLookup(${name})->${base}->${marks}`);
  for (let i = start + 1; i < lines.length;) {
    const l = trim(lines[i]);
    if (l === "END_SUBSTITUTION" || l === "END_POSITION") {
      console.log(`->endLookup();`);
      return i + 1;
    } else if (l === "EXCEPT_CONTEXT" || l === "IN_CONTEXT") {
      i = consumeContext(lines, i);
    } else if (l === "AS_SUBSTITUTION") {
      return consumeSubstitution(lines, i);
    } else if (l === "AS_POSITION") {
      return consumePosition(lines, i);
    } else {
      console.log(l);
      throw new Error();
    }
  }
  return lines.length;
};

const main = async () => {
  const vtp = (await fs.promises.readFile("EgyptianText_200.vtp")).toString("utf-8");
  const lines = vtp.split("\n");
  for (let i = 0; i < lines.length;) {
    const l = lines[i];
    if (l.startsWith("DEF_LOOKUP")) {
      i = consumeLookup(lines, i);
    } else {
      i += 1;
    }
  }
};

main();
